<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0"/>
  <title>Ishan DevLab</title>

  <style>
    :root {
      --bg: #0f172a;
      --panel: #1e293b;
      --text: #e5e7eb;
      --accent: #38bdf8;
    }

    body.light {
      --bg: #f8fafc;
      --panel: #e2e8f0;
      --text: #020617;
    }

    * {
      box-sizing: border-box;
      font-family: system-ui, sans-serif;
    }

    body {
      margin: 0;
      background: var(--bg);
      color: var(--text);
      display: grid;
      grid-template-columns: 220px 1fr;
      height: 100vh;
    }

    aside {
      background: var(--panel);
      padding: 1rem;
    }

    aside h2 {
      margin: 0 0 1rem;
      color: var(--accent);
    }

    aside button {
      width: 100%;
      padding: .6rem;
      margin-bottom: .5rem;
      border: none;
      background: transparent;
      color: var(--text);
      cursor: pointer;
      text-align: left;
    }

    aside button:hover {
      background: rgba(56,189,248,.15);
    }

    main {
      padding: 1rem;
      overflow: auto;
    }

    header {
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    .editor {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 1rem;
      margin-top: 1rem;
    }

    textarea {
      width: 100%;
      height: 200px;
      background: var(--panel);
      color: var(--text);
      border: none;
      padding: 1rem;
      resize: none;
    }

    iframe {
      width: 100%;
      height: 100%;
      border: none;
      background: white;
    }

    .hidden { display: none; }
  </style>
</head>

<body>
  <aside>
    <h2>DevLab</h2>
    <button data-page="dashboard">Dashboard</button>
    <button data-page="html">HTML</button>
    <button data-page="css">CSS</button>
    <button data-page="js">JavaScript</button>
    <button data-page="playground">Playground</button>
  </aside>

  <main>
    <header>
      <h1 id="title">Dashboard</h1>
      <button id="theme">Toggle Theme</button>
    </header>

    <section id="dashboard">
      <p>Advanced coding environment for real developers.</p>
    </section>

    <section id="html" class="hidden">
      <pre>&lt;section&gt;Semantic HTML&lt;/section&gt;</pre>
    </section>

    <section id="css" class="hidden">
      <pre>grid-template-columns: repeat(auto-fit, minmax(200px,1fr));</pre>
    </section>

    <section id="js" class="hidden">
      <pre>const app = () => module.init();</pre>
    </section>

    <section id="playground" class="hidden">
      <div class="editor">
        <textarea id="code">
<h1>Hello Dev</h1>
<style>
  h1 { color: purple; }
</style>
<script>
  console.log("Advanced mode");
</script>
        </textarea>
        <iframe id="output"></iframe>
      </div>
      <button onclick="run()">Run ▶</button>
    </section>
  </main>

  <script>
    const pages = document.querySelectorAll("section");
    const title = document.getElementById("title");

    document.querySelectorAll("aside button").forEach(btn => {
      btn.onclick = () => {
        pages.forEach(p => p.classList.add("hidden"));
        document.getElementById(btn.dataset.page).classList.remove("hidden");
        title.textContent = btn.textContent;
      };
    });

    document.getElementById("theme").onclick = () =>
      document.body.classList.toggle("light");

    function run() {
      document.getElementById("output").srcdoc =
        document.getElementById("code").value;
    }
  </script>
</body>
</html>
