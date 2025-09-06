// Updates metrics on Home page from SSE
const vEl = document.getElementById('home_v');
const iEl = document.getElementById('home_i');
const pEl = document.getElementById('home_p');

if(vEl && iEl && pEl){
  const es = new EventSource('/api/stream');
  es.onmessage = ev => {
    const m = JSON.parse(ev.data.replace(/'/g,'"'));
    vEl.textContent = `${m.Vrms_V.toFixed(2)} V`;
    iEl.textContent = `${m.Irms_A.toFixed(3)} A`;
    pEl.textContent = `${m.Pavg_W.toFixed(2)} W`;
  };
}