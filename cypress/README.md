# Cypress Tests

This directory contains Cypress end-to-end tests for the second-movement watch simulator.

## Setup

1. Install dependencies:
   ```bash
   npm install
   ```

2. Start a local web server to serve the HTML files:
   ```bash
   python3 -m http.server 8000
   ```
   Or use any other static file server on port 8000.

## Running Tests

### Interactive Mode
```bash
npm run cypress:open
```

### Headless Mode
```bash
npm run cypress:run
```

## Tests

### btn3-click.cy.js
Tests clicking button #btn3 in the watch simulator interface.

## Configuration

- **cypress.config.js**: Main Cypress configuration
- **cypress/support/**: Support files and custom commands
- **cypress/e2e/**: Test files
- **cypress/fixtures/**: Test data files (if needed)
