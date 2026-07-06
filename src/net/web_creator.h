// web_creator.h — embedded Recipe Creator page (roadmap §4.1.1). Single-page
// vanilla JS, served from PROGMEM. Builds/validates/saves recipe programs; the
// firmware validator is the source of truth (browser mirrors for instant
// feedback, device re-validates on save). No CDN — works offline.
#pragma once
#if !defined(PANPILOT_SIM)
#include <pgmspace.h>

static const char PANPILOT_CREATOR_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>PanPilot Recipe Creator</title>
<style>
body{margin:0;background:#101418;color:#f5f5f5;font-family:system-ui,sans-serif;padding:1rem}
h1{font-size:1.2rem}select,input,button{font-size:1rem;padding:.3rem;margin:.15rem;
 background:#1a2027;color:#f5f5f5;border:1px solid #2a323c;border-radius:6px}
button{cursor:pointer}table{width:100%;border-collapse:collapse;margin:.5rem 0}
td,th{border:1px solid #2a323c;padding:.3rem;font-size:.9rem}
#msg{padding:.5rem;border-radius:8px;margin:.5rem 0}.ok{background:#1b4d1f}.err{background:#5a1d1d}
</style></head><body>
<h1>Recipe Creator</h1>
<div><input id="name" placeholder="Recipe name" value="My Recipe">
 <select id="food"></select><button onclick="fromFood()">New from food</button></div>
<table id="steps"><thead><tr><th>#</th><th>type</th><th>a</th><th>b</th><th>expect</th><th>say</th><th></th></tr></thead>
<tbody></tbody></table>
<button onclick="addRow('HOLD',400,0,0,'')">+ step</button>
<button onclick="validate()">Validate</button>
<button onclick="save()">Save</button>
<div id="msg"></div>
<pre id="json"></pre>
<script>
const TYPES=['HOLD','CUE','TIMER','PREP','LOOP','END'];let foods=[];
fetch('/api/foodlib').then(r=>r.json()).then(f=>{foods=f;
 document.getElementById('food').innerHTML=f.map((x,i)=>`<option value="${i}">${x.name} ${x.variant}</option>`).join('');});
function addRow(t,a,b,e,say){const tb=document.querySelector('#steps tbody');
 const i=tb.rows.length;const tr=tb.insertRow();
 tr.innerHTML=`<td>${i}</td>
  <td><select>${TYPES.map(x=>`<option ${x==t?'selected':''}>${x}</option>`).join('')}</select></td>
  <td><input type=number value=${a} style=width:4rem></td>
  <td><input type=number value=${b} style=width:4rem></td>
  <td><input type=number value=${e} style=width:3rem></td>
  <td><input value="${say}"></td>
  <td><button onclick="this.closest('tr').remove();renum()">x</button></td>`;}
function renum(){[...document.querySelectorAll('#steps tbody tr')].forEach((r,i)=>r.cells[0].textContent=i);}
function fromFood(){const f=foods[document.getElementById('food').value];
 document.querySelector('#steps tbody').innerHTML='';
 addRow('HOLD',f.lo,0,0,'Preheat');
 addRow('CUE',0,120,1,'Add '+f.name);
 addRow('TIMER',f.side1,0,0,'Side 1');
 addRow('CUE',0,0,2,'Flip');
 addRow('TIMER',f.side2,0,0,'Side 2');
 addRow('CUE',0,0,2,'Remove'+(f.safe?' - verify '+f.safe+'F internal':''));
 addRow('END',0,0,0,'');renum();}
function collect(){const steps=[...document.querySelectorAll('#steps tbody tr')].map(r=>({
  type:r.cells[1].querySelector('select').value,a:+r.cells[2].querySelector('input').value,
  b:+r.cells[3].querySelector('input').value,expect:+r.cells[4].querySelector('input').value,
  say:r.cells[5].querySelector('input').value}));
 return {name:document.getElementById('name').value,steps};}
function show(o,ok){const m=document.getElementById('msg');m.className=ok?'ok':'err';m.textContent=o;}
function validate(){const p=collect();document.getElementById('json').textContent=JSON.stringify(p,null,1);
 fetch('/api/programs/validate',{method:'POST',body:JSON.stringify(p)}).then(r=>r.json())
 .then(v=>show(v.ok?'Valid ✓':'Invalid: '+v.reason+' (step '+v.badStep+')',v.ok));}
function save(){const p=collect();
 fetch('/api/programs/new',{method:'PUT',body:JSON.stringify(p)}).then(r=>r.json())
 .then(v=>show(v.ok?'Saved ✓':'Rejected: '+v.reason,v.ok));}
</script></body></html>)HTML";
#endif
