#!/usr/bin/env node

const path = require('path');
const fs = require('fs');
const { spawn } = require('child_process');

const platform = process.platform;
const arch = process.arch;

const PLATFORM_MAP = {
  darwin: {
    arm64: 'bobc-darwin-arm64',
    x64: 'bobc-darwin-x64'
  },
  linux: {
    x64: 'bobc-linux-x64',
    arm64: 'bobc-linux-arm64'
  },
  win32: {
    x64: 'bobc-win32-x64.exe',
    arm64: 'bobc-win32-x64.exe' // Windows on ARM can run x64 emulated
  }
};

const supportedArchs = PLATFORM_MAP[platform];
if (!supportedArchs) {
  console.error(`[bobc] Unsupported operating system: ${platform}`);
  console.error('Bob currently supports macOS (darwin), Linux, and Windows.');
  process.exit(1);
}

const binaryName = supportedArchs[arch];
if (!binaryName) {
  console.error(`[bobc] Unsupported architecture for ${platform}: ${arch}`);
  console.error('Supported architectures: x64, arm64.');
  process.exit(1);
}

let binaryPath = path.join(__dirname, 'bin', binaryName);

// Fallback for local development when running inside repository root
if (!fs.existsSync(binaryPath)) {
  const localDevBin = platform === 'win32'
    ? path.join(__dirname, '..', 'bobc.exe')
    : path.join(__dirname, '..', 'bobc');
  if (fs.existsSync(localDevBin)) {
    binaryPath = localDevBin;
  }
}

if (!fs.existsSync(binaryPath)) {
  console.error(`[bobc] Executable binary not found: ${binaryPath}`);
  console.error('Please ensure the package was installed correctly or build Bob from source.');
  console.error('Source & compilation instructions: https://github.com/ayomide/bob');
  process.exit(1);
}

// Ensure execution permissions on Unix platforms
if (platform !== 'win32') {
  try {
    const stats = fs.statSync(binaryPath);
    if ((stats.mode & 0o111) === 0) {
      fs.chmodSync(binaryPath, stats.mode | 0o755);
    }
  } catch (err) {
    // Non-fatal, attempt spawn anyway
  }
}

const args = process.argv.slice(2);
const child = spawn(binaryPath, args, {
  stdio: 'inherit',
  windowsHide: true
});

child.on('error', (err) => {
  console.error(`[bobc] Failed to start binary '${binaryPath}':`, err.message);
  process.exit(1);
});

child.on('exit', (code, signal) => {
  if (signal) {
    process.kill(process.pid, signal);
  } else {
    process.exit(code ?? 0);
  }
});
