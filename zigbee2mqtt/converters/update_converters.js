const https = require('https');
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const mqtt = require('mqtt');

// Automatically updates the custom converters
// for romasku firmware whenever Z2M starts.
// It creates 1 backup of the files before updating.

// Check the settings below and place it in
// zigbee2mqtt/data/external_converters directory

// WARNING!
// This script is very invasive!
// Use it at your own risk!
// (like any other scripts you get from the internet)


/************************** Settings ******************************/

// URL
const user = 'romasku';             // default: romasku
const repo = 'tuya-zigbee-switch';  // default: tuya-zigbee-switch
const branch = 'main';              // default: main

const url_tuya_with_ota = `https://raw.githubusercontent.com/${user}/${repo}/refs/heads/${branch}/zigbee2mqtt/converters/tuya_with_ota.js`;
const url_switch_custom = `https://raw.githubusercontent.com/${user}/${repo}/refs/heads/${branch}/zigbee2mqtt/converters/switch_custom.js`;

// Local path to the converters directory
const converters_path = 'data/external_converters';

// MQTT broker settings - needed to restart Z2M after update
const mqttUrl = 'mqtt://localhost:1883';
const mqttUsername = '';                    // optional
const mqttPassword = '';                    // optional


/************************** Script ********************************/

function md5(buffer) {
    return crypto.createHash('md5').update(buffer).digest('hex');
}

function download(url) {
    return new Promise((resolve, reject) => {
        https.get(url, (res) => {
            if (res.statusCode !== 200) {
                reject(new Error(`Failed to get ${url} (${res.statusCode})`));
                res.resume();
                return;
            }

            const data = [];
            res.on('data', chunk => data.push(chunk));
            res.on('end', () => resolve(Buffer.concat(data)));
        }).on('error', reject);
    });
}

async function update(url, fileName) {
    const filePath = path.join(converters_path, fileName);
    const remoteBuffer = await download(url);
    const remoteHash = md5(remoteBuffer);

    // Check for differences between local and remote files
    if (fs.existsSync(filePath)) {
        const localHash = md5(fs.readFileSync(filePath));
        if (remoteHash === localHash) {
            console.log(`No update needed for ${fileName}`);
            return false;
        }

        // Backup old file
        const backupPath = filePath + '.bak';
        fs.renameSync(filePath, backupPath);
        console.log(`Backup created: ${backupPath}`);
    }

    fs.writeFileSync(filePath, remoteBuffer);
    console.log(`Updated ${fileName}!`);
    return true;
}

function restartZ2MviaMQTT() {
    return new Promise((resolve, reject) => {
        const client = mqtt.connect(mqttUrl, {
            username: mqttUsername || undefined,
            password: mqttPassword || undefined
        });

        client.on('connect', () => {
            console.log('Connected to MQTT — sending restart command...');
            client.publish('zigbee2mqtt/bridge/request/restart', '', {}, (err) => {
                if (err) reject(err);
                else resolve();
                client.end();
            });
        });

        client.on('error', (err) => {
            reject(err);
        });
    });
}

(async () => {
    try {
        let updated = false;

        if (await update(url_tuya_with_ota, 'tuya_with_ota.js')) updated = true;
        if (await update(url_switch_custom, 'switch_custom.js')) updated = true;

        if (updated) {
            await restartZ2MviaMQTT();
            console.log('Converters updated — Zigbee2MQTT restart command sent over MQTT.');
        } else {
            console.log('No updates found — Zigbee2MQTT not restarted.');
        }
    } catch (err) {
        console.error(`${user}/${repo}/${branch} converters update failed:`, err.message);
    }
})();
