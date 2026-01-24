<!DOCTYPE html>
<html lang="de">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>Rechner</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; touch-action: none; }
        body { background: #000; font-family: -apple-system, sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; overflow: hidden; }

        /* RECHNER */
        #calc { width: 100%; max-width: 350px; padding: 20px; }
        #screen { width: 100%; height: 100px; color: white; text-align: right; font-size: 60px; padding: 10px; display: flex; align-items: flex-end; justify-content: flex-end; overflow: hidden; }
        .grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 12px; }
        button { height: 70px; border-radius: 50%; border: none; font-size: 26px; cursor: pointer; color: white; background: #333; }
        .op { background: #ff9f0a; }
        .spec { background: #a5a5a5; color: black; }
        button:active { filter: brightness(1.5); }

        /* GAME */
        #game-box { display: none; width: 100vw; height: 100vh; background: #050510; position: fixed; top:0; left:0; }
        canvas { width: 100%; height: 100%; display: block; }
        
        #back-btn { position: absolute; top: 25px; left: 20px; width: 45px; height: 45px; background: rgba(255,255,255,0.1); border-radius: 50%; display: flex; justify-content: center; align-items: center; color: white; font-size: 24px; z-index: 100; border: 1px solid rgba(255,255,255,0.2); }
        #ui { position: absolute; top: 30px; width: 100%; text-align: center; color: #00ffcc; font-family: monospace; font-size: 22px; pointer-events: none; }
        
        /* GAME OVER SCREEN */
        #game-over { display: none; position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); text-align: center; color: white; background: rgba(0,0,0,0.8); padding: 30px; border-radius: 20px; border: 2px solid #ff0066; z-index: 200; }
        .btn-restart { background: #ff0066; color: white; border: none; padding: 15px 30px; border-radius: 10px; font-size: 20px; margin-top: 15px; cursor: pointer; }

        .controls { position: absolute; bottom: 40px; width: 100%; display: flex; justify-content: center; gap: 40px; }
        .btn-game { width: 85px; height: 85px; background: rgba(0, 255, 204, 0.1); border-radius: 50%; display: flex; justify-content: center; align-items: center; font-size: 30px; color: #00ffcc; border: 2px solid #00ffcc; }
    </style>
</head>
<body>

    <div id="calc">
        <div id="screen">0</div>
        <div class="grid">
            <button class="spec" onclick="clr()">C</button><button class="spec" onclick="add('(')">(</button><button class="spec" onclick="add(')')">)</button><button class="op" onclick="add('/')">÷</button>
            <button class="num" onclick="add('7')">7</button><button class="num" onclick="add('8')">8</button><button class="num" onclick="add('9')">9</button><button class="op" onclick="add('*')">×</button>
            <button class="num" onclick="add('4')">4</button><button class="num" onclick="add('5')">5</button><button class="num" onclick="add('6')">6</button><button class="op" onclick="add('-')">-</button>
            <button class="num" onclick="add('1')">1</button><button class="num" onclick="add('2')">2</button><button class="num" onclick="add('3')">3</button><button class="op" onclick="add('+')">+</button>
            <button class="num" style="grid-column: span 2; width: 155px; border-radius: 40px;" onclick="add('0')">0</button><button class="num" onclick="add('.')">,</button><button class="op" onclick="check()">=</button>
        </div>
    </div>

    <div id="game-box">
        <div id="back-btn" onclick="exitGame()">✕</div>
        <div id="ui">DISTANZ: <span id="score">0</span>m</div>
        
        <div id="game-over">
            <h1>CRASH!</h1>
            <p id="final-score">Score: 0</p>
            <button class="btn-restart" onclick="resetGame()">Nochmal</button>
        </div>

        <canvas id="raceCanvas"></canvas>
        <div class="controls">
            <div class="btn-game" id="lBtn">◀</div>
            <div class="btn-game" id="rBtn">▶</div>
        </div>
    </div>

    <script>
        let expr = "";
        const scr = document.getElementById("screen");
        let gameActive = false;
        let isGameOver = false;
        let animationId;
        let objects = [];
        let score = 0;
        let speed = 4;
        let kartX = 0;

        function add(v) { if(expr === "0") expr = ""; expr += v; scr.innerText = expr.replace(/\*/g,'×').replace(/\//g,'÷') || "0"; }
        function clr() { expr = ""; scr.innerText = "0"; }
        
        function check() {
            if (expr === "76") { startGame(); return; }
            try { scr.innerText = eval(expr) || "0"; expr = scr.innerText; } catch { scr.innerText = "Error"; expr = ""; }
        }

        function startGame() {
            document.getElementById("calc").style.display="none"; 
            document.getElementById("game-box").style.display="block"; 
            resetGame();
        }

        function exitGame() {
            gameActive = false;
            cancelAnimationFrame(animationId);
            document.getElementById("game-box").style.display="none"; 
            document.getElementById("calc").style.display="block"; 
            clr();
        }

        function resetGame() {
            gameActive = true;
            isGameOver = false;
            document.getElementById("game-over").style.display = "none";
            objects = [];
            score = 0;
            speed = 4;
            const canvas = document.getElementById("raceCanvas");
            kartX = canvas.width / 2;
            if(!animationId) startRace();
        }

        function startRace() {
            const canvas = document.getElementById("raceCanvas");
            const ctx = canvas.getContext("2d");
            canvas.width = window.innerWidth;
            canvas.height = window.innerHeight;

            let moveL = false, moveR = false;
            document.getElementById("lBtn").onpointerdown = (e) => { e.preventDefault(); moveL = true; };
            document.getElementById("lBtn").onpointerup = () => moveL = false;
            document.getElementById("rBtn").onpointerdown = (e) => { e.preventDefault(); moveR = true; };
            document.getElementById("rBtn").onpointerup = () => moveR = false;

            function draw() {
                if (!gameActive) return;

                ctx.fillStyle = "#050510"; ctx.fillRect(0,0, canvas.width, canvas.height);
                
                // Straße
                ctx.fillStyle = "#1a1a2e";
                ctx.beginPath();
                ctx.moveTo(canvas.width*0.45, canvas.height*0.5);
                ctx.lineTo(canvas.width*0.55, canvas.height*0.5);
                ctx.lineTo(canvas.width * 1.2, canvas.height);
                ctx.lineTo(-canvas.width * 0.2, canvas.height);
                ctx.fill();

                if(!isGameOver) {
                    if(moveL && kartX > 40) kartX -= 6;
                    if(moveR && kartX < canvas.width - 40) kartX += 6;
                    score += 0.1;
                    if(Math.random() < 0.03) objects.push({ x: canvas.width/2 + (Math.random()-0.5)*50, y: canvas.height*0.5, s: 2 });
                }

                // Spieler
                ctx.fillStyle = "#ff0066";
                ctx.fillRect(kartX - 25, canvas.height - 140, 50, 80);

                for(let i = objects.length - 1; i >= 0; i--) {
                    let o = objects[i];
                    if(!isGameOver) {
                        o.y += speed; o.s += 0.8;
                        o.x += (o.x - canvas.width/2) * 0.07;
                    }

                    ctx.fillStyle = "#00ffcc";
                    ctx.fillRect(o.x - o.s/2, o.y, o.s, o.s);

                    if(!isGameOver && o.y > canvas.height - 160 && o.y < canvas.height - 60 && Math.abs(o.x - kartX) < (o.s/2 + 25)) {
                        isGameOver = true;
                        document.getElementById("game-over").style.display = "block";
                        document.getElementById("final-score").innerText = "Score: " + Math.floor(score);
                    }
                    if(o.y > canvas.height) objects.splice(i, 1);
                }

                document.getElementById("score").innerText = Math.floor(score);
                animationId = requestAnimationFrame(draw);
            }
            draw();
        }
    </script>
</body>
</html>
