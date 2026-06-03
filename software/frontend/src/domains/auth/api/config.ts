// Lê de VITE_API_BASE_URL no build/dev; default '/api' (proxy via vite.config.ts).
// `||` (não `??`) garante o default mesmo quando a env vem como string vazia,
// que é o caso do .env local (VITE_API_BASE_URL= sem valor).
export const API_BASE_URL: string = import.meta.env.VITE_API_BASE_URL || '/api';
