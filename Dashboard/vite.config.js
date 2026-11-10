import { defineConfig } from 'vite';
import { svelte } from '@sveltejs/vite-plugin-svelte';

export default defineConfig({
  plugins: [svelte()],
  server: {
    port: 5173,
    host: '0.0.0.0',
    proxy: {
      '/api/v1/business': {
        target: 'http://127.0.0.1:8081',
        changeOrigin: true
      },
      '/api': {
        target: 'http://127.0.0.1:8080',
        changeOrigin: true,
        ws: true
      }
    }
  }
});
