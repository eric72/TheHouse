import { defineConfig } from 'vite'

/** Racine = ce dossier (index.html, css/, fr/, en/). */
export default defineConfig({
  root: '.',
  server: {
    port: 5173,
    open: true,
  },
})
