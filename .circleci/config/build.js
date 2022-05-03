const cp = require('child_process');
const fs = require('fs-extra');
const path = require('path');
const yaml = require('js-yaml');

const STAGING_DIR = path.resolve(__dirname, '..', 'config-staging');

function copyAndExpand(dir = './') {
    const absDir = path.resolve(__dirname, dir);
    const targetDir = path.resolve(STAGING_DIR, dir);

    if (!fs.existsSync(targetDir)) {
        fs.mkdirSync(targetDir);
    }

    for (const file of fs.readdirSync(absDir)) {
        if (!file.endsWith('.yml')) {
            if (fs.statSync(path.resolve(absDir, file)).isDirectory()) {
                copyAndExpand(path.join(dir, file));
            }
            continue;
        }

        fs.writeFileSync(path.resolve(targetDir, file), yaml.dump(yaml.load(fs.readFileSync(path.resolve(absDir, file), 'utf8')), {
            noRefs: true,
        }));
    }
}

if (fs.pathExists(STAGING_DIR)) fs.removeSync(STAGING_DIR);
copyAndExpand();

const output = cp.spawnSync(process.env.CIRCLECI_BINARY || 'circleci', ['config', 'pack', STAGING_DIR]);
fs.writeFileSync(path.resolve(STAGING_DIR, 'built.yml'), output.stdout.toString());
