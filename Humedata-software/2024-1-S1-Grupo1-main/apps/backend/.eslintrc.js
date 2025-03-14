/** @type {import("eslint").Linter.Config} */
module.exports = {
  root: true,
  extends: ["@repo/eslint-config/library.js"],
  parser: "@typescript-eslint/parser", 
  ignorePatterns: ["src/generated"],
  parserOptions: {
    project: true,
  },
};
