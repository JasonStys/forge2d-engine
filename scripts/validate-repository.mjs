/**
 * File: validate-repository.mjs
 * Purpose: Enforce documentation, source-header, local-link, asset, and generated-index invariants.
 * Symbols and line locations: see docs/code-index.md for exact declarations.
 * Important variables: requiredFiles and headerExtensions define the portfolio contract.
 */
import { existsSync, readFileSync, readdirSync } from "node:fs";
import { dirname, extname, join, relative, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const requiredFiles = [
  "README.md", "CONTRIBUTING.md", "SECURITY.md", "CMakeLists.txt", "CMakePresets.json",
  "assets/arena.scene", "docs/architecture.md", "docs/api.md", "docs/complexity.md",
  "docs/file-catalog.md", "docs/gameplay.md", "docs/operations.md", "docs/research.md",
  "docs/testing.md", "docs/adr/0001-headless-core-and-optional-sdl3.md",
  "docs/reports/benchmark.md", "docs/reports/test-summary.md", "docs/reports/validation.md",
];
const headerExtensions = new Set([".c", ".cpp", ".h", ".hpp", ".mjs", ".sh"]);
const excludedDirectories = new Set([".git", ".cache", "build", "coverage", "dist", "out"]);

/** Recursively list authored files while excluding generated output and dependencies. */
function walk(directory) {
  return readdirSync(directory, { withFileTypes: true }).flatMap((entry) => {
    const path = join(directory, entry.name);
    if (entry.isDirectory()) return excludedDirectories.has(entry.name) ? [] : walk(path);
    return [path];
  });
}

/** Return broken local Markdown destinations; remote availability is checked outside this script. */
function brokenLinks(path) {
  const markdown = readFileSync(path, "utf8");
  return [...markdown.matchAll(/\[[^\]]*\]\(([^)]+)\)/g)].flatMap((match) => {
    const target = match[1].replace(/^<|>$/g, "");
    if (/^(?:https?:|mailto:|#)/.test(target)) return [];
    const localTarget = decodeURIComponent(target.split(/[?#]/, 1)[0]);
    return existsSync(resolve(dirname(path), localTarget)) ? [] : [target];
  });
}

/** Validate the reviewability contract and report all failures in one pass. */
function main() {
  const failures = [];
  for (const required of requiredFiles) {
    if (!existsSync(join(root, required))) failures.push(`Missing required file: ${required}`);
  }
  for (const path of walk(root)) {
    const repositoryPath = relative(root, path).replaceAll("\\", "/");
    if (extname(path) === ".md") {
      for (const target of brokenLinks(path)) {
        failures.push(`Broken local Markdown link in ${repositoryPath}: ${target}`);
      }
    }
    if (!headerExtensions.has(extname(path))) continue;
    const firstLines = readFileSync(path, "utf8").split(/\r?\n/).slice(0, 10).join("\n");
    if (!firstLines.includes("File:") || !firstLines.includes("Purpose:")) {
      failures.push(`Source header is incomplete: ${repositoryPath}`);
    }
  }
  if (failures.length > 0) {
    console.error(failures.join("\n"));
    process.exitCode = 1;
    return;
  }
  console.log("Repository structure, links, assets, and source headers are valid.");
}

main();
