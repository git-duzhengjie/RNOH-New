/**
 * RNOH-New: rnh-doctor.js
 *
 * Checks the development environment for RNOH-New.
 */

import { execSync } from 'child_process';
import fs from 'fs';
import path from 'path';
import os from 'os';
import chalk from 'chalk';

/**
 * Environment check result
 */
class CheckResult {
  name = '';
  status = '';
  version = '';
  message = '';

  constructor(name, status, version, message) {
    this.name = name;
    this.status = status;
    this.version = version;
    this.message = message;
  }
}

/**
 * Check Node.js version
 */
function checkNode() {
  try {
    const version = process.version;
    const major = parseInt(version.slice(1).split('.')[0], 10);
    const ok = major >= 18;
    return new CheckResult(
      'Node.js',
      ok ? 'PASS' : 'FAIL',
      version,
      ok ? 'Node.js 18+ is installed.' : 'Node.js 18+ is required.'
    );
  } catch (e) {
    return new CheckResult('Node.js', 'FAIL', '-', `Error: ${e.message}`);
  }
}

/**
 * Check hvigorw availability
 */
function checkHvigorw() {
  try {
    const output = execSync('hvigorw --version', { encoding: 'utf-8', stdio: 'pipe' }).trim();
    return new CheckResult('hvigorw', 'PASS', output, 'hvigorw is available.');
  } catch {
    return new CheckResult(
      'hvigorw',
      'FAIL',
      '-',
      'hvigorw not found. Add it to PATH or use ./harmony/hvigorw.'
    );
  }
}

/**
 * Check DevEco Studio installation
 */
function checkDevEcoStudio() {
  const platform = os.platform();
  const candidates = [];

  if (platform === 'win32') {
    candidates.push(
      path.join(os.homedir(), 'AppData', 'Local', 'Huawei', 'DevEco Studio'),
      'C:\\Program Files\\Huawei\\DevEco Studio',
      'C:\\Program Files (x86)\\Huawei\\DevEco Studio'
    );
  } else if (platform === 'darwin') {
    candidates.push('/Applications/DevEco Studio.app');
  } else {
    candidates.push('/opt/DevEco Studio');
  }

  for (const candidate of candidates) {
    if (fs.existsSync(candidate)) {
      return new CheckResult('DevEco Studio', 'PASS', candidate, 'DevEco Studio found.');
    }
  }

  return new CheckResult(
    'DevEco Studio',
    'WARN',
    '-',
    'DevEco Studio installation not found in common locations.'
  );
}

/**
 * Check HarmonyOS SDK
 */
function checkHarmonyOSSDK() {
  const platform = os.platform();
  const candidates = [];

  if (platform === 'win32') {
    candidates.push(
      path.join(os.homedir(), 'AppData', 'Local', 'OpenHarmony', 'Sdk'),
      path.join(os.homedir(), 'AppData', 'Local', 'Huawei', 'Sdk'),
      'C:\\Users\\Public\\Huawei\\Sdk'
    );
  } else if (platform === 'darwin') {
    candidates.push(
      path.join(os.homedir(), 'Library', 'OpenHarmony', 'Sdk'),
      path.join(os.homedir(), 'Library', 'Huawei', 'Sdk')
    );
  } else {
    candidates.push(
      path.join(os.homedir(), 'openharmony', 'sdk'),
      '/opt/huawei/sdk'
    );
  }

  for (const candidate of candidates) {
    if (fs.existsSync(candidate)) {
      return new CheckResult('HarmonyOS SDK', 'PASS', candidate, 'HarmonyOS SDK found.');
    }
  }

  return new CheckResult(
    'HarmonyOS SDK',
    'FAIL',
    '-',
    'HarmonyOS SDK not found. Set HOS_SDK_HOME or install the SDK.'
  );
}

/**
 * Print results as a table
 */
function printResults(results) {
  const nameWidth = 18;
  const statusWidth = 8;

  console.log(
    chalk.bold('Check'.padEnd(nameWidth)) +
    chalk.bold('Status'.padEnd(statusWidth)) +
    chalk.bold('Version/Path')
  );
  console.log('-'.repeat(70));

  for (const r of results) {
    const statusColor = r.status === 'PASS' ? chalk.green :
      r.status === 'WARN' ? chalk.yellow : chalk.red;
    console.log(
      r.name.padEnd(nameWidth) +
      statusColor(r.status.padEnd(statusWidth)) +
      r.version
    );
    console.log(chalk.gray(`  ${r.message}`));
  }
}

/**
 * Run the doctor command.
 */
function runDoctor() {
  console.log(chalk.blue('Checking RNOH-New environment...\n'));

  const results = [
    checkNode(),
    checkHvigorw(),
    checkDevEcoStudio(),
    checkHarmonyOSSDK(),
  ];

  printResults(results);

  const failed = results.filter((r) => r.status === 'FAIL');
  if (failed.length > 0) {
    console.log(chalk.red(`\n${failed.length} check(s) failed. Please fix the issues above.`));
    process.exit(1);
  } else {
    console.log(chalk.green('\nAll checks passed!'));
  }
}

export default runDoctor;
