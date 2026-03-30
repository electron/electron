<!DOCTYPE html>
<html lang="pt-br">
<head>
  <meta charset="UTF-8">
  <title>Starthits Radio</title>
  <style>
    body {
      margin: 0;
      font-family: Arial, sans-serif;
      background: linear-gradient(135deg, #0f2027, #203a43, #2c5364);
      color: white;
      text-align: center;
    }

    header {
      padding: 30px;
      font-size: 28px;
      font-weight: bold;
    }

    .player {
      margin-top: 50px;
      padding: 30px;
      background: rgba(255,255,255,0.1);
      border-radius: 15px;
      display: inline-block;
    }

    button {
      padding: 15px 25px;
      font-size: 18px;
      border: none;
      border-radius: 10px;
      cursor: pointer;
      margin: 10px;
    }

    .play {
      background: #00c853;
      color: white;
    }

    .stop {
      background: #d50000;
      color: white;
    }

    footer {
      margin-top: 50px;
      font-size: 14px;
      opacity: 0.7;
    }
  </style>
</head>
<body>

<header>📻 Starthits </header>

<div class="player">
  <p>Ao vivo agora 🎶</p>

  <audio id="radio" src="https://stream-176.zeno.fm/c08z6zzkmxhvv?zt=eyJhbGciOiJIUzI1NiJ9.eyJzdHJlYW0iOiJjMDh6Nnp6a214aHZ2IiwiaG9zdCI6InN0cmVhbS0xNzYuemVuby5mbSIsInJ0dGwiOjUsImp0aSI6IjJfaEhuX01XUmV5bGp6RDZRX0xVSUEiLCJpYXQiOjE3NzQ4NjA0OTcsImV4cCI6MTc3NDg2MDU1N30.716bRfmeqK1qKavEaEe_nasTEkI0CZDbdPUFEHOgLos"></audio>

  <br>

  <button class="play" onclick="playRadio()">▶ Play</button>
  <button class="stop" onclick="stopRadio()">⏹ Stop</button>
</div>

<footer>
  © 2026 - Starthits 
</footer>

<script>
  const radio = document.getElementById("radio");

  function playRadio() {
    radio.play();
  }

  function stopRadio() {
    radio.pause();
    radio.currentTime = 0;
  }
</script>

</body>
</html>
