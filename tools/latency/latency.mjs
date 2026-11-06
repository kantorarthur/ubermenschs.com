#!/usr/bin/env node
// Measures the latency of the API and saves the results.
//
//   node tools/latency/latency.mjs                         1000 requests to https://api.ubermenschs.com/api/health
//   node tools/latency/latency.mjs --count 200 --label "after deploy"
//   node tools/latency/latency.mjs --warm                  reuse one connection, like a browser does
//   node tools/latency/latency.mjs --url http://localhost:8080/api/health
//
// "cold" (default): every request opens a new connection, so it includes TCP and the TLS handshake.
// "warm" (--warm): one connection is kept open and reused, so it measures the request itself.
//
// The requests go one after another (latency, not load). Every run:
//   - prints p50 / p95 / p99 / max,
//   - adds one line to tools/latency/results/summary.csv (all runs, to compare them over time),
//   - saves every single request in tools/latency/results/raw/<date>_<mode>.csv.
//
// Only use read-only routes such as /api/health: login and register have rate limits.
// Needs only Node.js (the same as the frontend), no packages.

import fs from 'node:fs'
import http from 'node:http'
import https from 'node:https'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const RESULTS_DIR = path.join(path.dirname(fileURLToPath(import.meta.url)), 'results')

function parseArgs(argv) {
  const options = { url: 'https://api.ubermenschs.com/api/health', count: 1000, label: '', warm: false }
  for (let i = 0; i < argv.length; i++) {
    const arg = argv[i]
    if (arg === '--warm') {
      options.warm = true
    } else if (arg === '--url' || arg === '--count' || arg === '--label') {
      const value = argv[++i]
      if (value === undefined) {
        throw new Error(`${arg} needs a value`)
      }
      options[arg.slice(2)] = arg === '--count' ? Number(value) : value
    } else {
      throw new Error(`Unknown option: ${arg}`)
    }
  }
  if (!Number.isInteger(options.count) || options.count < 1) {
    throw new Error('--count must be a whole number of at least 1')
  }
  return options
}

// Milliseconds since `start`, from the high-resolution clock.
function since(start) {
  return Number(process.hrtime.bigint() - start) / 1e6
}

// One GET request, with the time of each step in ms from its start.
// dns / connect / tls are empty when an open connection was reused (or for http: no tls).
function measure(url, agent) {
  return new Promise((resolve) => {
    const start = process.hrtime.bigint()
    const times = { dns: '', connect: '', tls: '', ttfb: '', total: '', status: 0, error: '' }
    const client = url.protocol === 'https:' ? https : http

    const request = client.get(url, { agent }, (response) => {
      times.ttfb = since(start)
      times.status = response.statusCode
      response.on('data', () => {})
      response.on('end', () => {
        times.total = since(start)
        resolve(times)
      })
    })
    request.on('socket', (socket) => {
      // A reused connection (--warm) is already open: no lookup, connect or handshake to time.
      if (!socket.connecting) {
        return
      }
      socket.once('lookup', () => (times.dns = since(start)))
      socket.once('connect', () => (times.connect = since(start)))
      socket.once('secureConnect', () => (times.tls = since(start)))
    })
    request.setTimeout(10_000, () => request.destroy(new Error('timeout after 10 s')))
    request.on('error', (error) => {
      times.error = error.message
      resolve(times)
    })
  })
}

// Nearest-rank percentile of sorted numbers: the smallest value that p% of the samples do not exceed.
function percentile(sorted, p) {
  const index = Math.max(0, Math.ceil((p / 100) * sorted.length) - 1)
  return sorted[index]
}

function summarize(values) {
  const sorted = [...values].sort((a, b) => a - b)
  const average = sorted.reduce((sum, v) => sum + v, 0) / sorted.length
  return {
    p50: percentile(sorted, 50),
    p95: percentile(sorted, 95),
    p99: percentile(sorted, 99),
    max: sorted[sorted.length - 1],
    avg: average,
  }
}

const ms = (value) => (value === '' || value === undefined ? '' : value.toFixed(1))

function appendCsv(file, header, row) {
  const exists = fs.existsSync(file)
  fs.appendFileSync(file, (exists ? '' : header + '\n') + row + '\n')
}

function csvText(text) {
  return `"${String(text).replace(/"/g, '""')}"`
}

async function main() {
  const options = parseArgs(process.argv.slice(2))
  const url = new URL(options.url)
  const mode = options.warm ? 'warm' : 'cold'
  // warm: one connection kept open; cold: a new connection for every request.
  const agent = options.warm
    ? new (url.protocol === 'https:' ? https : http).Agent({ keepAlive: true, maxSockets: 1 })
    : false

  console.log(`${options.count} ${mode} requests to ${url.href}`)
  const samples = []
  for (let i = 0; i < options.count; i++) {
    samples.push(await measure(url, agent))
    if ((i + 1) % 100 === 0 || i + 1 === options.count) {
      process.stdout.write(`\r  ${i + 1}/${options.count}`)
    }
  }
  process.stdout.write('\n')
  if (agent) {
    agent.destroy()
  }

  const ok = samples.filter((s) => !s.error && s.status >= 200 && s.status < 400)
  const failed = samples.length - ok.length
  if (ok.length === 0) {
    console.error(`Every request failed, e.g.: ${samples[0].error || 'HTTP ' + samples[0].status}`)
    process.exitCode = 1
    return
  }

  const total = summarize(ok.map((s) => s.total))
  // Time to the first byte after the connection is ready: the server plus one round trip.
  const afterConnect = summarize(ok.map((s) => s.ttfb - (s.tls !== '' ? s.tls : s.connect !== '' ? s.connect : 0)))

  console.log('')
  console.log('                 p50      p95      p99      max      avg   (ms)')
  const line = (name, r) =>
    console.log(
      `  ${name.padEnd(13)}` + [r.p50, r.p95, r.p99, r.max, r.avg].map((v) => v.toFixed(1).padStart(9)).join(''),
    )
  line('total', total)
  line('first byte*', afterConnect)
  console.log('  * from the moment the connection was ready to the first byte of the answer')
  if (failed > 0) {
    console.log(`  ${failed} of ${samples.length} requests failed and are not counted`)
  }

  // Save.
  const now = new Date()
  const stamp = now.toISOString().replace(/[:.]/g, '-').slice(0, 23)
  fs.mkdirSync(path.join(RESULTS_DIR, 'raw'), { recursive: true })

  const rawFile = path.join(RESULTS_DIR, 'raw', `${stamp}_${mode}.csv`)
  const rawLines = ['request,status,error,dns_ms,connect_ms,tls_ms,first_byte_ms,total_ms']
  samples.forEach((s, i) =>
    rawLines.push([i + 1, s.status, csvText(s.error), ms(s.dns), ms(s.connect), ms(s.tls), ms(s.ttfb), ms(s.total)].join(',')),
  )
  fs.writeFileSync(rawFile, rawLines.join('\n') + '\n')

  const summaryFile = path.join(RESULTS_DIR, 'summary.csv')
  appendCsv(
    summaryFile,
    'date,label,url,mode,requests,failed,p50_ms,p95_ms,p99_ms,max_ms,avg_ms,first_byte_p50_ms,first_byte_p95_ms',
    [
      now.toISOString(),
      csvText(options.label),
      csvText(url.href),
      mode,
      samples.length,
      failed,
      ms(total.p50),
      ms(total.p95),
      ms(total.p99),
      ms(total.max),
      ms(total.avg),
      ms(afterConnect.p50),
      ms(afterConnect.p95),
    ].join(','),
  )

  console.log('')
  console.log(`Saved: ${path.relative(process.cwd(), summaryFile)} (one line per run)`)
  console.log(`       ${path.relative(process.cwd(), rawFile)} (every request)`)
}

main().catch((error) => {
  console.error(error.message)
  process.exitCode = 1
})
