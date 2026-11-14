"""build_rank_page.py -- writes docs/index.html, the rankings page served by GitHub Pages.

Input: docs/rankings_page.html (the page as published on claude.ai, written for a dashboard runtime) and the ranking
data that bench/rankings/merge_rank.py writes (merged.json, tables.json). The page's own scripts run unchanged on a
small stand-in for the four runtime calls they use (dash.data, dash.calc, dash.draw, dash.colors), with the data
inlined, so the page needs no server.
    python3 bench/rankings/merge_rank.py && python3 tools/build_rank_page.py"""
import json, os
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
page = open(os.path.join(ROOT, 'docs', 'rankings_page.html')).read()
page = page.replace('<meta name="dash-one-writer">\n', '')            # an editing marker of the dashboard runtime
merged = json.load(open(os.path.join(ROOT, 'bench', 'rankings', 'merged.json')))['rows']
tables = json.load(open(os.path.join(ROOT, 'bench', 'rankings', 'tables.json')))['rows']
inline = lambda o: json.dumps(o, separators=(',', ':')).replace('</', '<\\/')
TOKENS_LIGHT = """
    --color-bg: #fbfbf9; --color-fg: #1c1e22; --color-fg-muted: #5c616b; --color-panel: #f1f1ed;
    --color-border-line: #dcdcd5; --cds-border: #cfcfc7; --cds-text-secondary: #5c616b;
    --cds-chart-axis: #9a9ea6; --cds-chart-grid: #e7e7e1; --cds-surface-popover: #ffffff;
    --cds-shadow-popover: 0 4px 16px rgba(20, 22, 26, .12);"""
TOKENS_DARK = """
    --color-bg: #141518; --color-fg: #e9e9e6; --color-fg-muted: #a3a7af; --color-panel: #1e2024;
    --color-border-line: #2f3238; --cds-border: #3a3d44; --cds-text-secondary: #a3a7af;
    --cds-chart-axis: #6c7079; --cds-chart-grid: #26292e; --cds-surface-popover: #24262b;
    --cds-shadow-popover: 0 4px 16px rgba(0, 0, 0, .45);"""
HEAD = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Merged Compression Rankings</title>
<meta name="description" content="One ranking of the programs listed in both of Matt Mahoney's public compression benchmarks, the Large Text Compression Benchmark and the Silesia benchmark, with FluidCompressor's local measurement.">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Source+Sans+3:wght@400;500&family=Source+Serif+4:opsz,wght@8..60,400&display=swap">
<style>
  :root {%s
    --font-anthropic-sans: "Source Sans 3", -apple-system, "Segoe UI", Roboto, sans-serif;
    --font-anthropic-serif: "Source Serif 4", Georgia, "Times New Roman", serif;
    --cds-font-size-caption: 13px; --cds-font-size-body: 15px; --cds-font-size-heading: 19px; --cds-font-size-title: 34px;
    --cds-font-weight-medium: 500; --cds-radius: 8px; --cds-pad-lg: 18px;
    --cds-gap-xs: 4px; --cds-gap-sm: 8px; --cds-gap-md: 14px; --cds-gap-lg: 22px; --cds-gap-xl: 40px;
    color-scheme: light dark;
  }
  @media (prefers-color-scheme: dark) { :root {%s } }
  html, body { margin: 0; background: var(--color-bg); color: var(--color-fg); }
  body { font-family: var(--font-anthropic-sans); font-size: var(--cds-font-size-body); -webkit-font-smoothing: antialiased; }
  #dash-root { max-width: 1100px; margin: 0 auto; padding: 48px 24px 32px; box-sizing: border-box; }
  .gh-foot { max-width: 1100px; margin: 0 auto; padding: 0 24px 48px; box-sizing: border-box; color: var(--color-fg-muted); font-size: var(--cds-font-size-caption); line-height: 1.5; }
  .gh-foot a { color: inherit; }
  @media (max-width: 600px) { #dash-root { padding: 28px 16px 24px; } .gh-foot { padding: 0 16px 36px; } :root { --cds-font-size-title: 28px; } }
</style>
<script src="https://cdnjs.cloudflare.com/ajax/libs/d3/7.9.0/d3.min.js"></script>
<script type="application/json" id="data-merged">%s</script>
<script type="application/json" id="data-tables">%s</script>
<script>
  (function () {
    var sets = {}, calcs = {}, draws = [];
    ['merged', 'tables'].forEach(function (id) {
      var rows = JSON.parse(document.getElementById('data-' + id).textContent);
      sets[id] = {status: 'ok', data: rows, columns: rows.length ? Object.keys(rows[0]) : [], message: null, refreshing: false, truncated: false, meta: null};
    });
    function data(id) {
      if (sets[id]) return sets[id];
      var c = calcs[id];
      if (!c) return {status: 'missing', data: null, message: 'no source ' + id, refreshing: false, truncated: false, meta: null};
      if (!('value' in c)) c.value = c.fn.apply(null, c.inputs.map(function (i) { var d = data(i); return d.status === 'ok' ? d.data : []; }));
      return {status: 'ok', data: c.value, message: null, refreshing: false, truncated: false, meta: null};
    }
    function runDraws() {
      draws.forEach(function (d) { document.querySelectorAll('[data-draw="' + d.name + '"]').forEach(function (el) { d.fn(el); }); });
    }
    window.dash = {
      data: data,
      calc: function (id, o) { calcs[id] = {inputs: o.inputs || [], fn: o.fn}; },
      draw: function (name, fn) { if (fn) draws.push({name: name, fn: fn}); else runDraws(); },
      onData: function (fn) { fn(); window.addEventListener('resize', fn); },
      colors: ['#8a909b', '#c2410c', '#2563eb', '#16a34a'],
      params: function () { return {}; }, setParams: function () {}, resetParams: function () {}, setLink: function () {},
      refresh: function () {}, loader: function () {},
      _start: function () {
        runDraws();
        var t, w = window.innerWidth;
        window.addEventListener('resize', function () { clearTimeout(t); t = setTimeout(function () { if (window.innerWidth !== w) { w = window.innerWidth; runDraws(); } }, 120); });
      }
    };
  })();
</script>
</head>
<body>
<div id="dash-root" lang="en">
"""
FOOT = """
</div>
<footer class="gh-foot">
  Built from <a href="https://github.com/devkancheti4-design/fluidcompressor">github.com/devkancheti4-design/fluidcompressor</a>:
  the data comes from <code>bench/rankings/merge_rank.py</code> over the two benchmark tables as saved, and this page from
  <code>tools/build_rank_page.py</code>.
</footer>
<script>dash._start();</script>
</body>
</html>
"""
out = HEAD % (TOKENS_LIGHT, TOKENS_DARK, inline(merged), inline(tables)) + page + FOOT
open(os.path.join(ROOT, 'docs', 'index.html'), 'w').write(out)
print('wrote docs/index.html (%d bytes, %d merged rows, %d tables)' % (len(out), len(merged), len(tables)))
