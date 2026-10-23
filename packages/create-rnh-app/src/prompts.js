/**
 * RNOH-New: create-rnh-app 交互式提问
 *
 * 优先使用 inquirer（若已安装），否则使用 Node.js 内置 readline 兜底。
 * 避免将 inquirer 作为硬依赖。
 */

const readline = require('node:readline');
const { validateProjectName } = require('./utils');

let inquirer = null;
try {
  inquirer = require('inquirer');
} catch {
  inquirer = null; // 未安装 inquirer，使用 readline 兜底
}

/**
 * 判断是否可用 inquirer
 *
 * @returns {boolean}
 */
function hasInquirer() {
  return inquirer !== null;
}

/**
 * 使用 readline 提问单个问题
 *
 * @param {string} question 问题文本
 * @param {string} defaultValue 默认值
 * @param {Function} validate 校验函数（返回 true 或错误字符串）
 * @returns {Promise<string>} 用户输入
 */
function askWithReadline(question, defaultValue, validate) {
  const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
  });

  return new Promise((resolve) => {
    const ask = () => {
      const hint = defaultValue ? ` (${defaultValue})` : '';
      rl.question(`${question}${hint}: `, (answer) => {
        const value = answer.trim() || defaultValue;
        if (validate) {
          const result = validate(value);
          if (result !== true) {
            console.log(`  ✗ ${result}`);
            ask();
            return;
          }
        }
        rl.close();
        resolve(value);
      });
    };
    ask();
  });
}

/**
 * 询问项目名
 *
 * @param {string} defaultValue 默认项目名
 * @returns {Promise<string>} 项目名
 */
async function askProjectName(defaultValue) {
  const validate = (name) => {
    if (!validateProjectName(name)) {
      return '项目名不合法：仅允许小写字母、数字、连字符、下划线，且不以数字开头。';
    }
    return true;
  };

  if (inquirer) {
    const { projectName } = await inquirer.prompt([
      {
        type: 'input',
        name: 'projectName',
        message: '项目名称:',
        default: defaultValue,
        validate,
      },
    ]);
    return projectName;
  }

  return askWithReadline('请输入项目名称', defaultValue, validate);
}

/**
 * 询问模板类型
 *
 * @param {string} defaultValue 默认模板名
 * @param {string[]} templates 可用模板列表
 * @returns {Promise<string>} 模板名
 */
async function askTemplate(defaultValue, templates) {
  const validate = (name) => {
    if (!templates.includes(name)) {
      return `模板不存在，可选：${templates.join(', ')}`;
    }
    return true;
  };

  if (inquirer) {
    const { template } = await inquirer.prompt([
      {
        type: 'list',
        name: 'template',
        message: '选择模板:',
        choices: templates,
        default: defaultValue,
      },
    ]);
    return template;
  }

  console.log(`可用的模板: ${templates.join(', ')}`);
  return askWithReadline('请输入模板名称', defaultValue, validate);
}

/**
 * 询问是否跳过依赖安装
 *
 * @param {boolean} defaultValue 默认值
 * @returns {Promise<boolean>} 是否跳过安装
 */
async function askSkipInstall(defaultValue) {
  if (inquirer) {
    const { skipInstall } = await inquirer.prompt([
      {
        type: 'confirm',
        name: 'skipInstall',
        message: '是否跳过依赖安装?',
        default: defaultValue,
      },
    ]);
    return skipInstall;
  }

  const answer = await askWithReadline(
    '是否跳过依赖安装 (y/n)',
    defaultValue ? 'y' : 'n',
    () => true
  );
  return answer.toLowerCase() === 'y';
}

module.exports = {
  askProjectName,
  askTemplate,
  askSkipInstall,
  hasInquirer,
};