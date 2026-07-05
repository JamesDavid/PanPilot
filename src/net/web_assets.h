// web_assets.h — the embedded single-page web UI (roadmap spec §2.2). Served
// from PROGMEM (LittleFS+gzip is a later refinement). Connects a WebSocket for
// the live dashboard + a browser-side thermal view using the same ironbow map.
#pragma once
#if !defined(PANPILOT_SIM)
#include <pgmspace.h>

static const char PANPILOT_INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>PanPilot</title>
<style>
:root{color-scheme:dark}
body{margin:0;background:#101418;color:#f5f5f5;font-family:system-ui,sans-serif;
 display:flex;flex-direction:column;align-items:center;padding:1rem}
h1{font-size:1.2rem;color:#8a93a0;margin:.2rem}
#temp{font-size:5rem;font-weight:700;line-height:1}
#bar{width:100%;max-width:520px;text-align:center;padding:.6rem;border-radius:10px;
 font-size:1.3rem;margin:.6rem 0;background:#2a323c}
.row{color:#b7c0cc;margin:.15rem}
canvas{width:384px;max-width:96vw;image-rendering:pixelated;border-radius:8px;background:#000;margin-top:.6rem}
#conn{font-size:.8rem;color:#8a93a0}
</style></head><body>
<h1 id="mode">PanPilot</h1>
<div id="temp">--</div>
<div class="row"><span id="rate"></span> &middot; <span id="eta"></span></div>
<div id="bar">--</div>
<div class="row">target <span id="target">--</span></div>
<canvas id="cv" width="32" height="24"></canvas>
<div id="conn">connecting...</div>
<script>
const COLS=32,ROWS=24;
const cv=document.getElementById('cv'),cx=cv.getContext('2d');
const img=cx.createImageData(COLS,ROWS);
function iron(t){t=Math.max(0,Math.min(1,t));
 const s=[[0,0,0,0],[.15,40,0,80],[.33,130,0,130],[.5,200,30,30],
  [.7,240,120,0],[.88,255,220,40],[1,255,255,255]];
 let i=0;while(i<s.length-1&&t>s[i+1][0])i++;
 const a=s[i],b=s[i+1],f=(t-a[0])/((b[0]-a[0])||1);
 return[a[1]+(b[1]-a[1])*f,a[2]+(b[2]-a[2])*f,a[3]+(b[3]-a[3])*f];}
function drawThermal(px){let lo=1e9,hi=-1e9;for(const v of px){if(v<lo)lo=v;if(v>hi)hi=v;}
 const span=(hi-lo)>1?hi-lo:1;
 for(let k=0;k<px.length;k++){const c=iron((px[k]-lo)/span);
  img.data[k*4]=c[0];img.data[k*4+1]=c[1];img.data[k*4+2]=c[2];img.data[k*4+3]=255;}
 cx.putImageData(img,0,0);}
const BARC={HEAT_MORE:'#2E5AAC',HOLD:'#2E5AAC',TURN_DOWN_SOON:'#C08A00',
 TURN_DOWN_NOW:'#E07000',READY:'#2E7D32',TOO_HOT:'#C62828',COOLING:'#2E5AAC',
 RECOVERING:'#2E5AAC',CHECK_AIM:'#C08A00'};
function connect(){const ws=new WebSocket('ws://'+location.host+'/ws');
 ws.onopen=()=>document.getElementById('conn').textContent='live';
 ws.onclose=()=>{document.getElementById('conn').textContent='reconnecting...';setTimeout(connect,1500);};
 ws.onmessage=e=>{const m=JSON.parse(e.data);
  if(m.t==='s'){document.getElementById('temp').textContent=m.temp+'°'+m.u;
   document.getElementById('rate').textContent=m.rate+'°'+m.u+'/min';
   document.getElementById('eta').textContent=m.eta;
   document.getElementById('target').textContent=m.tgt+'°'+m.u;
   document.getElementById('mode').textContent=m.preset;
   const b=document.getElementById('bar');b.textContent=m.bar;
   b.style.background=BARC[m.g]||'#2a323c';}
  else if(m.t==='f'){drawThermal(m.px);}};}
connect();
</script></body></html>)HTML";
#endif
