// Copy this file and MinHeapAutoTest.cpp into cpp/tests/.
// Run from the repository: node cpp/tests/run-tests.mjs
// Optional for reviewing this pack: --repo <path> --heap-test <path>
import assert from 'node:assert/strict';
import { spawn, spawnSync } from 'node:child_process';
import { mkdtemp, mkdir, copyFile, writeFile, readFile, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { resolve, join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { setTimeout as delay } from 'node:timers/promises';
import net from 'node:net';

const here = dirname(fileURLToPath(import.meta.url));
const args = process.argv.slice(2);
function option(name, fallback) {
  const index = args.indexOf(name);
  return index < 0 ? fallback : args[index + 1];
}
const repo = resolve(option('--repo', join(here, '../..')));
const heapTest = resolve(option('--heap-test', join(here, 'MinHeapAutoTest.cpp')));
const compiler = process.env.CXX || 'g++';
const base = 'http://localhost:8081';
const root = await mkdtemp(join(tmpdir(), 'warehouse-system-test-'));
const unitDir = join(root, 'unit');
const systemDir = join(root, 'system');
const core = ['cpp/DSAcore/SearchCore.cpp', 'cpp/DSAcore/hashtable/hashtable.cpp',
  'cpp/DSAcore/trie/trie.cpp', 'cpp/DSAcore/Min_heap/Min_heap.cpp'];
const csvHeader = 'id,product_name,made_date,arrived_time,best_by_date,status\n';
const persistent = join(systemDir, 'cpp/DSAcore/Persistent/Persistent.csv');
let server = null;
let socket = null;
let messages = [];
let serverLogs = '';

function run(command, args, cwd = repo) {
  const result = spawnSync(command, args, { cwd, encoding: 'utf8', timeout: 120000 });
  if (result.stdout) process.stdout.write(result.stdout);
  if (result.stderr) process.stderr.write(result.stderr);
  assert.ifError(result.error);
  assert.equal(result.status, 0, `${command} failed with exit ${result.status}`);
}

function build(name, sources, serverBuild = false) {
  const exe = join(root, name + '.exe');
  run(compiler, ['-std=c++17', '-O0', '-g', '-Wall', '-Wextra',
    '-I' + join(repo, 'cpp/DSAcore/Min_heap'),
    ...sources.map(p => resolve(repo, p)), '-o', exe,
    ...(serverBuild ? ['-pthread', '-lws2_32'] : [])]);
  return exe;
}

async function portOccupied(host) {
  return new Promise(resolve => {
    const connection = net.createConnection({ host, port: 8081 });
    connection.setTimeout(1000);
    connection.once('connect', () => { connection.destroy(); resolve(true); });
    connection.once('error', () => { connection.destroy(); resolve(false); });
    connection.once('timeout', () => { connection.destroy(); resolve(false); });
  });
}

async function request(method, path, body, expected = 200, json = true) {
  const response = await fetch(base + path, {
    method,
    headers: { 'Content-Type': json ? 'application/json' : 'text/plain' },
    body: body === undefined ? undefined : json ? JSON.stringify(body) : body,
    signal: AbortSignal.timeout(5000)
  });
  assert.equal(response.status, expected, `${method} ${path}: HTTP ${response.status}`);
  return response.status === 204 ? null : response.json();
}

async function waitMessage(predicate) {
  const deadline = Date.now() + 5000;
  while (Date.now() < deadline) {
    const index = messages.findIndex(predicate);
    if (index >= 0) return messages.splice(index, 1)[0];
    if (server?.exitCode !== null) throw new Error('Server exited before WebSocket result');
    await delay(20);
  }
  throw new Error('Timed out waiting for expected WebSocket message');
}

async function startServer(exe) {
  messages = [];
  server = spawn(exe, [], { cwd: systemDir, windowsHide: true, stdio: ['ignore', 'pipe', 'pipe'] });
  server.stdout.on('data', data => { serverLogs += data; });
  server.stderr.on('data', data => { serverLogs += data; });
  let startupError;
  server.on('error', error => { startupError = error; });
  const deadline = Date.now() + 10000;
  let ready = false;
  while (Date.now() < deadline) {
    if (startupError) throw startupError;
    assert.equal(server.exitCode, null, 'Server exited during startup');
    try {
      await request('GET', '/search/autocomplete?prefix=Seed');
      ready = true;
      break;
    } catch { await delay(100); }
  }
  assert(ready, 'Server did not become ready');
  socket = new WebSocket('ws://localhost:8081/ws');
  socket.addEventListener('message', event => messages.push(JSON.parse(event.data)));
  await new Promise((resolve, reject) => {
    const timer = setTimeout(() => reject(new Error('WebSocket connection timeout')), 5000);
    socket.addEventListener('open', () => { clearTimeout(timer); resolve(); }, { once: true });
    socket.addEventListener('error', () => { clearTimeout(timer); reject(new Error('WebSocket connection error')); }, { once: true });
  });
  const initial = await waitMessage(message => message.type === 'recent_products');
  assert.deepEqual(initial.items, [], 'Recent must be empty on a fresh process');
}

async function closeSocket() {
  if (!socket || socket.readyState === WebSocket.CLOSED) return;
  const current = socket;
  const closed = new Promise(resolve => current.addEventListener('close', resolve, { once: true }));
  current.close();
  await Promise.race([closed, delay(3000)]);
  assert.equal(current.readyState, WebSocket.CLOSED, 'WebSocket must close before shutdown');
  socket = null;
}

async function stopServer() {
  await closeSocket();
  await request('POST', '/search/input', '/SHUTDOWN', 204, false);
  const deadline = Date.now() + 10000;
  while (server.exitCode === null && Date.now() < deadline) await delay(50);
  assert.equal(server.exitCode, 0, 'Server must exit normally and run atexit/saveCSV');
  server = null;
}

async function search(query, expectedIds) {
  messages = [];
  await request('POST', '/search/input', query, 204, false);
  const result = await waitMessage(message => message.type === 'search_result' && message.query === query);
  assert.deepEqual(result.results.map(product => product.id), expectedIds);
  return result.results;
}

async function recent(id, operation) {
  const result = await waitMessage(message => message.type === 'recent_products' &&
    message.items[0]?.product?.id === id && message.items[0]?.operation === operation);
  assert(result.items[0].time, 'Recent must include timestamp');
}

async function systemTest(exe) {
  // The fixed server path resolves against this temporary working directory.
  await mkdir(join(systemDir, 'cpp/DSAcore/Persistent'), { recursive: true });
  const seed = csvHeader + 'P00001,Seed Item,2026-01-01,2026-01-02,2099-01-01,AVAILABLE\n';
  await writeFile(join(systemDir, 'cpp/product_inventory_10 000.csv'), seed);
  await startServer(exe);
  assert.equal(await readFile(persistent, 'utf8'), seed, 'First startup must create persistent data');

  messages = [];
  const added = await request('POST', '/product/add', {
    product_name: 'System Item', made_date: '2026-01-01', arrived_time: '2026-01-02',
    best_by_date: '2099-01-01', quantity: 2
  }, 201);
  assert.equal(added.success, true);
  assert.equal(added.products.length, 2);
  const [keep, remove] = added.products;
  assert.notEqual(keep.id, remove.id);
  await recent(remove.id, 'ADD');
  assert.deepEqual(await request('GET', '/search/autocomplete?prefix=System'), ['System Item']);
  await search(keep.id, [keep.id]);
  await recent(keep.id, 'READ');
  await search('System', [keep.id, remove.id]);

  messages = [];
  const deleted = await request('DELETE', '/product/delete', { id: remove.id });
  assert.equal(deleted.success, true);
  assert.equal(deleted.id, remove.id);
  await waitMessage(message => message.type === 'search_result' &&
    message.query === 'System' && message.results.length === 1 && message.results[0].id === keep.id);
  await recent(remove.id, 'DELETE');
  await search(remove.id, []);
  await request('DELETE', '/product/delete', { id: remove.id }, 404);
  await request('POST', '/product/add', {}, 400);
  await request('GET', '/search/autocomplete', undefined, 400);
  await stopServer();

  const saved = await readFile(persistent, 'utf8');
  assert(saved.includes(keep.id + ',System Item,'), 'Added product must be saved');
  assert(!saved.includes(remove.id + ','), 'Deleted product must not be saved');
  await startServer(exe);
  const restored = await search(keep.id, [keep.id]);
  assert.deepEqual(restored[0], keep, 'All six Product fields must survive restart');
  await recent(keep.id, 'READ');
  await search(remove.id, []);
  await search('P00001', ['P00001']);
  await search('System', [keep.id]);
  await stopServer();
  console.log('PASS: HTTP + WebSocket Add -> Search -> Recent -> Delete -> Save -> Restart');
}

try {
  assert.equal(typeof WebSocket, 'function', 'Use Node.js 22 or newer');
  assert(!await portOccupied('127.0.0.1') && !await portOccupied('::1'),
    'Port 8081 is busy. Close your own demo server before running tests.');
  await mkdir(join(unitDir, 'cpp'), { recursive: true });
  for (const size of ['10 000', '100 000']) {
    const file = `product_inventory_${size}.csv`;
    await copyFile(join(repo, 'cpp', file), join(unitDir, 'cpp', file));
  }
  const tests = [
    ['SearchCoreTest', ['cpp/DSAcore/SearchCoreTest.cpp', ...core], unitDir],
    ['MinHeapAutoTest', [heapTest, 'cpp/DSAcore/Min_heap/Min_heap.cpp'], unitDir],
    ['LRUCacheTest', ['cpp/DSAcore/LRU_Cache/LRU_CacheTest.cpp'], unitDir]
  ];
  for (const [name, sources, cwd] of tests) {
    await mkdir(cwd, { recursive: true });
    console.log(`\nRunning ${name}`);
    run(build(name, sources), [], cwd);
  }
  await systemTest(build('warehouse-server', ['cpp/server.cpp', ...core], true));
  console.log('ALL TESTS PASSED');
} catch (error) {
  console.error('FAIL:', error.message);
  if (serverLogs) console.error(serverLogs);
  process.exitCode = 1;
} finally {
  if (socket) {
    try { await closeSocket(); } catch {}
  }
  if (server && server.exitCode === null) {
    try { await stopServer(); } catch {
      // Terminate only the disposable process created by this test.
      server.kill();
      await delay(500);
    }
  }
  await rm(root, { recursive: true, force: true });
}
