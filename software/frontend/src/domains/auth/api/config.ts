// Lê de VITE_API_BASE_URL no build/dev; default '/api' (proxy via vite.config.ts).
export const API_BASE_URL: string = import.meta.env.VITE_API_BASE_URL ?? '/api';
