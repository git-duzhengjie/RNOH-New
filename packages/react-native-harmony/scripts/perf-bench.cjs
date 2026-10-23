#!/usr/bin/env node
/**
 * RNOH-New: 性能基准测试脚本
 *
 * 模拟大量组件创建/更新/销毁，统计耗时并输出对比表格。
 * 用于快速评估桥接层性能，发现性能回归。
 *
 * 用法：
 *   node scripts/perf-bench.js
 *   node scripts/perf-bench.js --iterations 1000
 *   node scripts/perf-bench.js --json
 */

'use strict';

const os = require('node:os');
const path = require('node:path');

// ---------- 参数解析 ----------

const args = process.argv.slice(2);
let iterations = 5000;
let outputJson = false;

for (let i = 0; i < args.length; i++) {
  if (args[i] === '--iterations' && args[i + 1]) {
    iterations = parseInt(args[i + 1], 10);
    i++;
  } else if (args[i] === '--json') {
    outputJson = true;
  } else if (args[i] === '--help' || args[i] === '-h') {
    console.log(`
RNOH-New 性能基准测试

用法:
  node scripts/perf-bench.js [options]

选项:
  --iterations <n>   迭代次数，默认 5000
  --json             输出 JSON 格式结果
  -h, --help         显示帮助
`);
    process.exit(0);
  }
}

// ---------- 工具函数 ----------

/**
 * 高精度计时（毫秒）
 */
function nowMs() {
  const [sec, nano] = process.hrtime();
  return sec * 1000 + nano / 1e6;
}

/**
 * 运行基准测试
 * @param {string} name 测试名称
 * @param {Function} fn 测试函数
 * @param {number} count 迭代次数
 * @returns {{ name: string, totalMs: number, avgUs: number, opsPerSec: number }}
 */
function bench(name, fn, count) {
  // 预热
  for (let i = 0; i < Math.min(100, count); i++) {
    fn(i);
  }
  // 强制 GC（如果可用）
  if (global.gc) global.gc();

  const start = nowMs();
  for (let i = 0; i < count; i++) {
    fn(i);
  }
  const totalMs = nowMs() - start;

  const avgUs = (totalMs * 1000) / count;
  const opsPerSec = count / (totalMs / 1000);

  return {
    name,
    totalMs: Number(totalMs.toFixed(3)),
    avgUs: Number(avgUs.toFixed(2)),
    opsPerSec: Math.round(opsPerSec),
  };
}

/**
 * 格式化数字（加千分位）
 */
function formatNumber(n) {
  return n.toLocaleString('en-US');
}

// ---------- 模拟组件模型 ----------

/**
 * 模拟的组件实例（简化版）
 * 用于模拟 RNHComponentInstance 的创建/更新/销毁开销
 */
class MockComponent {
  constructor(id, type, props) {
    this.id = id;
    this.type = type;
    this.props = { ...props };
    this.children = [];
    this.parent = null;
    this.nativeNode = { id, type, refCount: 1 };
  }

  updateProps(newProps) {
    this.props = { ...this.props, ...newProps };
    this.nativeNode.props = { ...this.nativeNode.props, ...newProps };
  }

  addChild(child) {
    child.parent = this;
    this.children.push(child);
  }

  removeChild(child) {
    const idx = this.children.indexOf(child);
    if (idx !== -1) {
      this.children.splice(idx, 1);
      child.parent = null;
    }
  }

  destroy() {
    this.nativeNode.refCount--;
    this.children = [];
    this.props = null;
    this.parent = null;
  }
}

// ---------- 测试场景 ----------

function runBenchmarks() {
  const results = [];

  // 1. 简单对象创建
  results.push(
    bench('对象创建（空对象）', () => {
      return {};
    }, iterations)
  );

  // 2. 组件实例创建（模拟 View 组件）
  results.push(
    bench('组件创建（View）', (i) => {
      return new MockComponent(i, 'View', {
        flex: 1,
        backgroundColor: '#ffffff',
        width: 100,
        height: 100,
      });
    }, iterations)
  );

  // 3. 组件 props 更新（5 个属性）
  {
    const comp = new MockComponent(0, 'View', {});
    results.push(
      bench('Props 更新（5 个属性）', () => {
        comp.updateProps({
          width: Math.random() * 100,
          height: Math.random() * 100,
          opacity: Math.random(),
          backgroundColor: '#abcdef',
          borderRadius: Math.random() * 20,
        });
      }, iterations)
    );
    comp.destroy();
  }

  // 4. 组件 props 更新（20 个属性）
  {
    const comp = new MockComponent(0, 'View', {});
    const manyProps = {};
    for (let i = 0; i < 20; i++) {
      manyProps[`prop${i}`] = i;
    }
    results.push(
      bench('Props 更新（20 个属性）', () => {
        comp.updateProps(manyProps);
      }, iterations)
    );
    comp.destroy();
  }

  // 5. 子节点添加（每次添加 1 个）
  {
    const parent = new MockComponent(0, 'View', {});
    let childId = 0;
    results.push(
      bench('子节点添加（单个）', () => {
        const child = new MockComponent(childId++, 'View', {});
        parent.addChild(child);
        return child;
      }, iterations)
    );
    parent.destroy();
  }

  // 6. 子节点移除（每次移除 1 个）
  {
    const parent = new MockComponent(0, 'View', {});
    const children = [];
    for (let i = 0; i < iterations; i++) {
      const child = new MockComponent(i, 'View', {});
      parent.addChild(child);
      children.push(child);
    }
    results.push(
      bench('子节点移除（单个）', (i) => {
        parent.removeChild(children[i]);
        children[i].destroy();
      }, iterations)
    );
    parent.destroy();
  }

  // 7. 组件销毁
  {
    const comps = [];
    for (let i = 0; i < iterations; i++) {
      comps.push(new MockComponent(i, 'View', { a: 1, b: 2, c: 3 }));
    }
    results.push(
      bench('组件销毁', (i) => {
        comps[i].destroy();
      }, iterations)
    );
  }

  // 8. 深层嵌套创建（3 层）
  results.push(
    bench('深层嵌套创建（3 层）', (i) => {
      const root = new MockComponent(i, 'View', {});
      const l1 = new MockComponent(i * 10 + 1, 'View', {});
      const l2 = new MockComponent(i * 10 + 2, 'Text', {});
      const l3 = new MockComponent(i * 10 + 3, 'Image', {});
      root.addChild(l1);
      l1.addChild(l2);
      l2.addChild(l3);
      return root;
    }, Math.floor(iterations / 10))
  );

  // 9. JSON 序列化/反序列化（模拟桥接数据传输）
  {
    const data = {
      type: 'View',
      props: { flex: 1, backgroundColor: '#fff', width: 100, height: 200 },
      children: [{ type: 'Text', props: { text: 'hello' } }],
    };
    results.push(
      bench('JSON 序列化/反序列化', () => {
        const str = JSON.stringify(data);
        return JSON.parse(str);
      }, iterations)
    );
  }

  // 10. 数组操作（模拟子节点 reorder）
  {
    const arr = [];
    for (let i = 0; i < 100; i++) arr.push(i);
    results.push(
      bench('数组重排（100 元素）', () => {
        // 模拟一次 reorder：头部元素移到尾部
        const first = arr.shift();
        arr.push(first);
      }, iterations)
    );
  }

  return results;
}

// ---------- 输出 ----------

function printTable(results) {
  const sep = '-'.repeat(78);
  console.log(sep);
  console.log(
    `RNOH-New 性能基准测试  |  迭代: ${formatNumber(iterations)}  |  Node ${process.version}`
  );
  console.log(`平台: ${os.platform()} ${os.arch()}  |  CPU: ${os.cpus()[0].model}`);
  console.log(sep);
  console.log(
    `${'测试场景'.padEnd(28)}${'总耗时(ms)'.padStart(12)}${'单次(μs)'.padStart(12)}${'每秒操作'.padStart(14)}`
  );
  console.log(sep);

  for (const r of results) {
    console.log(
      `${r.name.padEnd(28)}` +
        `${formatNumber(r.totalMs).padStart(12)}` +
        `${formatNumber(r.avgUs).padStart(12)}` +
        `${formatNumber(r.opsPerSec).padStart(14)}`
    );
  }

  console.log(sep);
}

function printJson(results) {
  const output = {
    iterations,
    platform: os.platform(),
    arch: os.arch(),
    cpu: os.cpus()[0].model,
    nodeVersion: process.version,
    results,
  };
  console.log(JSON.stringify(output, null, 2));
}

// ---------- 主流程 ----------

try {
  const results = runBenchmarks();

  if (outputJson) {
    printJson(results);
  } else {
    printTable(results);
  }

  process.exit(0);
} catch (e) {
  console.error('性能测试失败:', e.message);
  console.error(e.stack);
  process.exit(1);
}
