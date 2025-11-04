const { defineConfig } = require('cypress')

module.exports = defineConfig({
  e2e: {
    // Configure baseUrl to serve files from the project root
    // You can start a local server with: python3 -m http.server 8000
    baseUrl: 'http://localhost:8000',
    specPattern: 'cypress/e2e/**/*.cy.{js,jsx,ts,tsx}',
    supportFile: 'cypress/support/e2e.js',
    video: false,
    screenshotOnRunFailure: true,
    setupNodeEvents(on, config) {
      // implement node event listeners here
    },
  },
})
