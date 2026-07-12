// web_creator.h — embedded Recipe Creator page (roadmap §4.1.1). Single-page
// vanilla JS, served from PROGMEM. Builds/validates/saves recipe programs; the
// firmware validator is the source of truth (browser mirrors for instant
// feedback, device re-validates on save). No CDN — works offline.
//
// The step editor is TYPE-AWARE (bench 2026-07-11: raw a/b/expect number
// columns were "confusing — what is a and b?"). Each step type shows labeled
// inputs for only the fields it uses; the a/b/expect encoding is rebuilt at
// collect() time and never shown to the user.
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
td.params{color:#b7bec8}td.params input[type=number]{width:4.5rem}
label{white-space:nowrap;margin-right:.4rem}
#msg{padding:.5rem;border-radius:8px;margin:.5rem 0}.ok{background:#1b4d1f}.err{background:#5a1d1d}
.hint{color:#8a93a0;font-size:.85rem}
</style></head><body>
<h1>Recipe Creator</h1>
<div><input id="name" placeholder="Recipe name" value="My Recipe">
 <select id="food"></select><button onclick="fromFood()">New from food</button></div>
<table id="steps"><thead><tr><th>#</th><th>step</th><th>what it does</th><th>cue text (shown on the device)</th><th></th></tr></thead>
<tbody></tbody></table>
<button onclick="addRow('HOLD',400,0,0,'');renum()">+ step</button>
<button onclick="validate()">Validate</button>
<button onclick="save()">Save</button>
<p class="hint">HOLD waits for the pan to reach a temperature &middot; CUE waits for you
(and/or a timeout) &middot; TIMER counts down on the device &middot; PREP adds a fat and
watches its smoke point &middot; LOOP repeats earlier steps &middot; END finishes.</p>
<div id="msg"></div>
<pre id="json"></pre>
<h1>New food (single-food timer)</h1>
<p class="hint">Adds to the device's food picker (or overrides a food with the same
name + variant). Times are seconds per side at the middle of the temp band.
The safe-internal-temp note can never be lowered below the USDA minimum for a
built-in food.</p>
<div>
 <input id="fn" placeholder="Name" style="width:10rem">
 <input id="fv" placeholder="Variant" style="width:12rem">
 <input id="flo" type="number" placeholder="lo F" style="width:5rem">
 <input id="fhi" type="number" placeholder="hi F" style="width:5rem"><br>
 <input id="fs1" type="number" placeholder="side 1 s" style="width:6rem">
 <input id="fs2" type="number" placeholder="side 2 s (0 = no flip)" style="width:11rem">
 <input id="fsafe" type="number" placeholder="safe internal F (0 = visual)" style="width:14rem"><br>
 <input id="fflip" placeholder="Flip hint shown on the cue" style="width:24rem">
 <button onclick="saveFood()">Save food to device</button>
</div>
<div id="fmsg"></div>
<script>
const TYPES=['HOLD','CUE','TIMER','PREP','LOOP','END'];let foods=[],preps=[];
fetch('/api/foodlib').then(r=>r.json()).then(f=>{foods=f;
 document.getElementById('food').innerHTML=f.map((x,i)=>`<option value="${i}">${x.name} ${x.variant}</option>`).join('');});
fetch('/api/preplib').then(r=>r.json()).then(p=>preps=p);
function paramsHtml(t,a,b,e){switch(t){
 case 'HOLD':return `hold the pan at <input data-f="a" type="number" value="${a}"> &deg;F until it gets there`;
 case 'CUE':return `advance when: <label><input data-f="food" type="checkbox" ${e&1?'checked':''}>food hits the pan</label>
  <label><input data-f="tap" type="checkbox" ${e&2?'checked':''}>you tap the bar</label>
  or after <input data-f="b" type="number" value="${b}"> s (0 = wait forever)`;
 case 'TIMER':return `count down <input data-f="a" type="number" value="${a}"> seconds (live on the device)`;
 case 'PREP':return `add fat: <select data-f="a">${preps.map((p,i)=>`<option value="${i}" ${i==a?'selected':''}>${p.name}</option>`).join('')}</select>
  &mdash; PanPilot watches its add window + smoke point`;
 case 'LOOP':return `jump back to step <input data-f="a" type="number" value="${a}" style="width:3.5rem">
  and repeat <input data-f="b" type="number" value="${b}" style="width:3.5rem"> more times`;
 default:return 'finishes the program';}}
function retype(sel){sel.closest('tr').querySelector('.params').innerHTML=paramsHtml(sel.value,0,0,0);}
function addRow(t,a,b,e,say){const tb=document.querySelector('#steps tbody');
 const i=tb.rows.length;const tr=tb.insertRow();
 tr.innerHTML=`<td>${i}</td>
  <td><select onchange="retype(this)">${TYPES.map(x=>`<option ${x==t?'selected':''}>${x}</option>`).join('')}</select></td>
  <td class="params">${paramsHtml(t,a,b,e)}</td>
  <td><input value="${say}"></td>
  <td><button onclick="this.closest('tr').remove();renum()">x</button></td>`;}
function renum(){[...document.querySelectorAll('#steps tbody tr')].forEach((r,i)=>r.cells[0].textContent=i);}
function fromFood(){const f=foods[document.getElementById('food').value];
 document.querySelector('#steps tbody').innerHTML='';
 addRow('HOLD',f.lo,0,0,'Preheat');
 addRow('CUE',0,120,3,'Add '+f.name);
 addRow('TIMER',f.side1,0,0,'Side 1');
 addRow('CUE',0,0,2,'Flip');
 addRow('TIMER',f.side2,0,0,'Side 2');
 addRow('CUE',0,0,2,'Remove'+(f.safe?' - verify '+f.safe+'F internal':''));
 addRow('END',0,0,0,'');renum();}
function collect(){const steps=[...document.querySelectorAll('#steps tbody tr')].map(r=>{
 const t=r.cells[1].querySelector('select').value;
 const q=f=>r.querySelector(`[data-f="${f}"]`);
 let a=0,b=0,e=0;
 if(t=='HOLD'||t=='TIMER'||t=='PREP')a=+q('a').value||0;
 else if(t=='LOOP'){a=+q('a').value||0;b=+q('b').value||0;}
 else if(t=='CUE'){b=+q('b').value||0;e=(q('food').checked?1:0)|(q('tap').checked?2:0);}
 return {type:t,a,b,expect:e,say:r.cells[3].querySelector('input').value};});
 return {name:document.getElementById('name').value,steps};}
function show(o,ok){const m=document.getElementById('msg');m.className=ok?'ok':'err';m.textContent=o;}
function validate(){const p=collect();document.getElementById('json').textContent=JSON.stringify(p,null,1);
 fetch('/api/programs/validate',{method:'POST',body:JSON.stringify(p)}).then(r=>r.json())
 .then(v=>show(v.ok?'Valid ✓':'Invalid: '+v.reason+' (step '+v.badStep+')',v.ok));}
function save(){const p=collect();
 fetch('/api/programs/new',{method:'PUT',body:JSON.stringify(p)}).then(r=>r.json())
 .then(v=>show(v.ok?'Saved ✓ - it now appears under Recipe programs on the device':'Rejected: '+v.reason,v.ok));}
function saveFood(){
 const g=id=>document.getElementById(id);const num=id=>+g(id).value||0;
 const s1=num('fs1'),s2=num('fs2');
 const f={name:g('fn').value,variant:g('fv').value,
  panLoF:num('flo'),panHiF:num('fhi'),
  sideSec:s2>0?[s1,s2]:[s1],sides:s2>0?2:1,
  refF:Math.round((num('flo')+num('fhi'))/2),compPct:-10,restSec:0,
  safeInternalF:num('fsafe'),flip:g('fflip').value};
 const m=document.getElementById('fmsg');
 if(!f.name||!f.panLoF||f.panHiF<=f.panLoF||s1<=0){
  m.className='err';m.textContent='need a name, lo < hi temps, and a side-1 time';return;}
 fetch('/api/foods',{method:'POST',body:JSON.stringify(f)}).then(r=>r.json())
 .then(v=>{m.className=v.ok?'ok':'err';
  m.textContent=v.ok?'Saved ✓ - it is in the device food picker now':'Rejected: '+(v.reason||'error');})
 .catch(()=>{m.className='err';m.textContent='device unreachable';});}
</script></body></html>)HTML";
#endif
