<!-- Modals -->
  <div class="modal-backdrop" id="roomModal"><div class="modal"><div id="roomManagementView"><h3>إدارة الغرفة</h3><div class="room-info" id="roomInfoBlock">رمز الغرفة الحالي: <span id="modalRoomId"></span></div><div class="row2" style="margin-bottom: 10px;"><button class="btn" id="openLog">📜 عرض السجل</button><button class="btn" id="copyLinkBtn">🔗 نسخ رابط المشاهدة</button></div><hr><div class="row2"><button class="btn" id="doCreate">🚀 إنشاء غرفة جديدة</button><button class="btn" id="showJoinView">🚪 الانضمام لغرفة أخرى</button></div></div><div id="joinView" style="display: none;"><h3>الانضمام لغرفة</h3><div style="display:flex; gap:6px; align-items:center;"><input type="text" id="joinId" class="input" placeholder="أدخل رمز الغرفة" /><button class="btn" id="doJoin">دخول</button></div><button class="btn" id="backToManagement" style="margin-top: 10px; background: #2a3257;">رجوع</button></div><hr><div style="display:flex; justify-content:flex-end;"><button class="btn" id="closeRoomModal">إغلاق</button></div></div></div>
  
  <!-- Log Page -->
  <div class="modal-backdrop" id="logPage" style="display: none;">
    <div class="log-page">
      <div class="log-header">
        <h2>📊 سجل النتائج</h2>
      </div>
      <div class="log-content">
        <table class="log-table">
          <thead>
            <tr>
              <th>📅</th>
              <th>👤</th>
              <th>🏆</th>
              <th>🥈</th>
              <th>🥇</th>
            </tr>
          </thead>
          <tbody id="logTableBody"></tbody>
        </table>
      </div>
      <div class="log-footer">
        <button class="btn log-footer-btn" id="clearLogBtn">🗑️ مسح السجل</button>
        <button class="btn log-footer-btn" id="closeLogPage">❌ إغلاق</button>
      </div>
    </div>
  </div>

  <!-- Clear Log Confirmation -->
  <div class="modal-backdrop" id="clearLogModal">
    <div class="modal">
      <h3>🗑️ تأكيد مسح السجل</h3>
      <p style="text-align: center; margin: 16px 0; color: var(--muted);">هل أنت متأكد من حذف جميع السجلات نهائياً؟</p>
      <div class="row2">
        <button class="btn" id="confirmClearLog" style="background-color: #e74c3c; color: white;">✔️ نعم، احذف</button>
        <button class="btn" id="cancelClearLog" style="background: #2a3257; color: white;">❌ إلغاء</button>
      </div>
    </div>
  </div>
  
  <div class="modal-backdrop" id="deleteModal"><div class="modal"><h3>🗑️ تأكيد الحذف</h3><p style="text-align: center; margin: 16px 0; color: var(--muted);">هل أنت متأكد من حذف هذا اللاعب؟</p><div class="row2" style="margin-bottom: 10px;"><button class="btn" id="deletePlayerBtn" style="background-color: #e74c3c; color: white;">حذف اللاعب</button><button class="btn" id="deleteAllBtn" style="background-color: #c0392b; color: white;">حذف الجميع</button></div><hr><div style="display:flex; justify-content:flex-end;"><button class="btn" id="closeDeleteModal" style="background: #2a3257; color: white;">إلغاء</button></div></div></div>
  
  <div class="modal-backdrop" id="devInfoModal"><div class="modal"><h3>معلومات المطور</h3><div class="info-item"><span>🧑‍💻</span> <b>Mr.K</b></div><div class="info-item"><span>📞</span> <a href="tel:+96899678553">99678553</a><a href="https://wa.me/96899678553" target="_blank" style="font-size: 24px; margin-right: auto;">💬</a></div><div class="info-item"><span>✉️</span> <a href="mailto:scoon80@gmail.com">scoon80@gmail.com</a></div><hr><p style="text-align:center;">شكراً لاستخدامكم التطبيق</p><div style="display:flex; justify-content:flex-end;"><button class="btn" id="closeDevInfoModal">إغلاق</button></div></div></div>

  <!-- نافذة إضافة اللاعبين -->
  <div class="modal-backdrop" id="addPlayerModal">
    <div class="modal">
      <h3>✨ إضافة لاعبين جدد</h3>
      <p class="subtle" style="text-align: center; margin-top: -10px; margin-bottom: 16px;">
        أدخل أسماء اللاعبين مفصولة بمسافة أو سطر جديد.
      </p>
      
      <textarea id="playerNamesInput" class="input" rows="5" placeholder="علي ماهر معاذ..."></textarea>
      
      <hr>
      
      <div class="row2">
        <button class="btn" id="doAddPlayers" style="background-color: #27ae60;">✔️ إضافة</button>
        <button class="btn" id="closeAddPlayerModal" style="background-color: #c0392b;">❌ إلغاء</button>
      </div>
    </div>
  </div>

  <script src="https://cdn.jsdelivr.net/npm/canvas-confetti@1.9.3/dist/confetti.browser.min.js"></script>
  <script type="module">
    import { initializeApp } from "https://www.gstatic.com/firebasejs/10.12.4/firebase-app.js";
    import { getDatabase, ref, set, onValue, update, get, child, remove } from "https://www.gstatic.com/firebasejs/10.12.4/firebase-database.js";

    const firebaseConfig = {
      apiKey: "AIzaSyBGuiMxffwcm_P8zY-I6hMUG1hUY1akpFA",
      authDomain: "uno-game-live.firebaseapp.com",
      databaseURL: "https://uno-game-live-default-rtdb.firebaseio.com",
      projectId: "uno-game-live",
      storageBucket: "uno-game-live.appspot.com",
      messagingSenderId: "973862287578",
      appId: "1:973862287578:web:0bac6d3788c78666e48de3"
    };

    const app = initializeApp(firebaseConfig);
    const db = getDatabase(app);

    const qs = new URLSearchParams(location.search);
    let roomId = qs.get("room") || localStorage.getItem("uno-room-id") || "";
    let adminToken = qs.get("admin") || JSON.parse(localStorage.getItem("uno-admin-tokens") || '{}')[roomId];
    let isAdmin = false;

    const headerContainer = document.getElementById("header-container");
    const winnerContainer = document.getElementById("winner-container");
    const playersEl = document.getElementById("players");
    const modalRoomId = document.getElementById("modalRoomId");
    let currentTargetPlayerId = null;

    // الأصوات
    const clickSound = new Audio('./click.mp3');
    clickSound.volume = 0.8;
    const winSound = new Audio('./fireworks.mp3');
    winSound.volume = 0.7;
    const silverMedalSound = new Audio('./silver.mp3');
    silverMedalSound.volume = 0.6;
    const goldMedalSound = new Audio('./gold.mp3');
    const victoryJingleSound = new Audio('./cup.mp3');
    goldMedalSound.volume = 0.6;
    victoryJingleSound.volume = 0.6;

    let soundsUnlocked = false;
    function unlockSounds() {
        if (soundsUnlocked) return;
        winSound.load();
        silverMedalSound.load();
        goldMedalSound.load();
        clickSound.load();
        soundsUnlocked = true;
        console.log("Audio context unlocked by user interaction.");
    }

    const ALL_EMOJIS = ['🤠','😎','🥷','🤡','🦸','🦹','👽','🤖','🐱','🦅','🐯','🐺','🐲','🦊','🐸','🐵','🦉','🐨','👨‍🚀','👩‍🚀','👨‍🎨','👩‍🎨','👨‍💻','👩‍💻','👨‍🔬','👩‍🔬'];
    const CARD_COLORS = ["#8e44ad", "#e67e22", "#c0392b", "#2980b9", "#27ae60", "#f39c12", "#16a085"];

    // دالة للحصول على شخصية غير مستخدمة
    function getAvailableCharacter(usedChars) {
        let charIndex = 1;
        let selectedChar = `images/uno${charIndex}.png`;
        
        // نبحث عن أول شخصية غير مستخدمة
        while (usedChars.includes(selectedChar)) {
            charIndex++;
            selectedChar = `images/uno${charIndex}.png`;
            
            // حماية من اللوب اللا نهائي - إذا وصلنا لرقم عالي جداً
            if (charIndex > 10000) {
                // نختار رقم عشوائي كبير ونأمل ألا يكون مستخدماً
                charIndex = Math.floor(Math.random() * 999999) + 10001;
                selectedChar = `images/uno${charIndex}.png`;
                break;
            }
        }
        
        return selectedChar;
    }

    const makeId = () => Math.random().toString(36).slice(2, 8);
    const todayTag = () => new Date().toLocaleDateString('en-GB',{day:'2-digit',month:'short'}).replace(/ /g,'').toUpperCase();

    const openModal = (m)=> m.style.display = "flex";
    const closeModal = (m)=> m.style.display = "none";

    function setRandomGradientBackground() {
      const gradients = [
        "linear-gradient(135deg, #0f2027, #203a43, #2c5364)", "linear-gradient(135deg, #13192e, #232d4b, #3e4a6d)",
        "linear-gradient(135deg, #23074d, #cc5333)", "linear-gradient(135deg, #1f1c2c, #928dab)",
        "linear-gradient(135deg, #16222a, #3a6073)", "linear-gradient(135deg, #373b44, #4286f4)",
        "linear-gradient(135deg, #000000, #434343)", "radial-gradient(circle, #2c3e50, #000000)"
      ];
      document.body.style.background = gradients[Math.floor(Math.random() * gradients.length)];
    }

    function hexToRgba(hex, alpha = 1) {
        const [r, g, b] = hex.match(/\w\w/g).map(x => parseInt(x, 16));
        return `rgba(${r},${g},${b},${alpha})`;
    }

    function getMedals(score) {
        const gold = Math.floor((score || 0) / 5);
        const silver = (score || 0) % 5;
        return "🥇".repeat(gold) + "🥈".repeat(silver);
    }

    function renderHeader() {
        const roomStatusText = isAdmin ? 'مشرف' : 'مشاهد';
        const bannerHtml = `
            <div class="header" id="roomBanner">
              <img id="bannerImage" src="./banner.jpg" alt="UNO Live Banner" class="banner-img" onerror="this.src='https://via.placeholder.com/600x120/8e44ad/ffffff?text=UNO+LIVE'">
              <div class="title">
                <span>🎮</span>
                <span class="badge" id="roomStatus">${roomStatusText}</span>
              </div>
            </div>`;
        headerContainer.innerHTML = bannerHtml;
        document.getElementById("roomBanner").onclick = () => {
            const roomModal = document.getElementById("roomModal");
            const roomManagementView = document.getElementById('roomManagementView');
            const joinView = document.getElementById('joinView');
            const roomInfoBlock = document.getElementById('roomInfoBlock');
            roomManagementView.style.display = 'block';
            joinView.style.display = 'none';
            const hasRoom = !!roomId;
            roomInfoBlock.style.display = hasRoom ? 'block' : 'none';
            document.getElementById('openLog').style.display = hasRoom ? 'block' : 'none';
            document.getElementById('copyLinkBtn').style.display = hasRoom ? 'block' : 'none';
            openModal(roomModal);
        };
    }

    // دالة إنشاء بطاقة الفائز المحدثة
    function winnerCard(p) {
        const cardColor = p.color || CARD_COLORS[0];
        const medalIcon = (p.left >= 5) ? "🥇" : "🥈";
        
        return `
            <div class="winner-section">
                <div class="winner-card" data-player-id="${p.id}" style="background: linear-gradient(135deg, ${hexToRgba(cardColor, 0.2)}, ${hexToRgba(cardColor, 0.1)});">
                    <div class="card-body">
                        <!-- الأيقونات في الأعلى: ميدالية يسار، كأس يمين -->
                        <!-- مثال لتعديل بطاقة الفائز -->
                        <div class="top-icons">
                        <span class="top-icon medal">${medalIcon}</span>
                        <span class="top-icon trophy">🏆</span>
                        </div>
                        
                        <div class="center-col">
                            <div class="emoji">
                                <img src="${p.character||'images/uno1.png'}" class="avatar" alt="char" onerror="this.src='images/uno1.png'">
                            </div>
                        </div>
                    </div>
                    
                    <!-- الترتيب الجديد: الميداليات يسار، الاسم وسط، الكأس يمين -->
                    <div class="card-header">
                        <div class="score-area left">
                            <div class="score-card" data-side="left" data-player-id="${p.id}">${p.left||0}</div>
                        </div>
                        <div class="player-name-section">
                            <span>${p.name||'لاعب'}</span>
                        </div>
                        <div class="score-area right">
                            <div class="score-card" data-side="right" data-player-id="${p.id}">${p.right||0}</div>
                        </div>
                    </div>
                </div>
            </div>`;
    }

    // دالة إنشاء بطاقة اللاعب العادي المحدثة
    function playerCard(p, index){
      const cardColor = p.color || CARD_COLORS[index % CARD_COLORS.length];
      const medalIcon = (p.left >= 5) ? "🥇" : "🥈";
      
      return `
        <div class="card" data-player-id="${p.id}" style="background-color: ${hexToRgba(cardColor, 0.8)};">
          <div class="card-body">
            <!-- الأيقونات في الأعلى: ميدالية يسار، كأس يمين -->
            <div class="top-icons">
              <span class="top-icon">${medalIcon}</span>
              <span class="top-icon">🏆</span>
            </div>
            
            <div class="center-col">
              <div class="emoji">
                <img src="${p.character||'images/uno1.png'}" class="avatar" alt="char" onerror="this.src='images/uno1.png'">
              </div>
            </div>
          </div>
          
          <!-- الترتيب الجديد: الميداليات يسار، الاسم وسط، الكأس يمين -->
          <div class="card-header">
            <div class="score-area left">
              <div class="score-card" data-side="left" data-player-id="${p.id}">${p.left||0}</div>
            </div>
            <div class="player-name-section">
              <span>${p.name||'لاعب'}</span>
            </div>
            <div class="score-area right">
              <div class="score-card" data-side="right" data-player-id="${p.id}">${p.right||0}</div>
            </div>
          </div>
        </div>`;
    }

    function renderPlayers(list){
      renderHeader();
      
      if (list.length === 0) {
        winnerContainer.innerHTML = '';
        playersEl.innerHTML = '';
        return;
      }

      // فصل الفائز عن باقي اللاعبين
      const winner = list[0]; // أول لاعب (أعلى الكؤوس)
      const otherPlayers = list.slice(1); // باقي اللاعبين
      
      // عرض الفائز في الأعلى
      if (winner && (winner.right > 0 || winner.left > 0)) {
        winnerContainer.innerHTML = winnerCard(winner);
      } else {
        winnerContainer.innerHTML = '';
      }
      
      // عرض باقي اللاعبين
      playersEl.innerHTML = otherPlayers.map((p, i) => playerCard(p, i + 1)).join("");
    }

    function triggerFireworks(winData) {
        const flashElement = document.getElementById('screen-flash');
        const canvas = document.getElementById('confetti-canvas');

        function runFlashEffect() {
            const flashCount = 3; 
            const flashInterval = 120;
            for (let i = 0; i < flashCount; i++) {
                setTimeout(() => {
                    flashElement.classList.add('active');
                    setTimeout(() => { flashElement.classList.remove('active'); }, 60);
                }, i * flashInterval);
            }
        }

        runFlashEffect();
        canvas.classList.add('active');
        const winnerColor = winData.color || '#FFFFFF';
        
        winSound.play().catch(e => {});
        victoryJingleSound.play().catch(e => {});

        const fire = confetti.create(canvas, { resize: true, useWorker: true });
        const duration = 5000;
        const end = Date.now() + duration;

        setTimeout(runFlashEffect, 3000);
        setTimeout(() => { canvas.classList.remove('active'); }, duration);

        (function frame() {
            if (Date.now() > end) return;
            const bannerRect = document.getElementById('roomBanner')?.getBoundingClientRect() || { bottom: 0 };
            fire({
                particleCount: 50, angle: 90, spread: 45, startVelocity: 55,
                origin: { x: Math.random(), y: 1 }, ticks: 150, gravity: 1,
                shapes: ['circle'], scalar: 0.6, colors: ['#FFFFFF', winnerColor]
            }).then(result => {
                if (!result || (bannerRect.bottom > 0 && (result.origin.y * window.innerHeight) < bannerRect.bottom)) return;
                fire({
                    particleCount: 800, spread: 360, startVelocity: 40, origin: result.origin,
                    gravity: 0.4, ticks: 500, decay: 0.94, shapes: ['circle'],
                    scalar: 2.0, colors: [winnerColor, '#FFFFFF', '#FFD700']
                });
            });
            setTimeout(frame, 500 + Math.random() * 300);
        }());
    }

    async function ensureAdminStatus(){
      if(!roomId || !adminToken) { isAdmin = false; return false; }
      const snap = await get(child(ref(db, `rooms/${roomId}`), 'adminKey'));
      isAdmin = !!(snap.exists() && adminToken === snap.val());
      document.getElementById('addPlayerFab').style.display = isAdmin ? 'flex' : 'none';
    }

    function handleScoreUpdate(id, side, delta) {
        if (!isAdmin) return;
        const playerRef = ref(db, `rooms/${roomId}/players/${id}`);
        get(playerRef).then(pSnap => {
            if(!pSnap.exists()) return;
            const p = pSnap.val();
            const oldValue = p[side] || 0;
            const newValue = Math.max(0, oldValue + delta);
            let updates = {};
            updates[`players/${id}/${side}`] = newValue;
            
            // تسجيل فقط المركز الأول (الكؤوس)
            if (side === 'right' && delta > 0 && newValue > oldValue) {
                const date = todayTag();
                const silverMedals = p.left || 0;
                const goldMedals = Math.floor(silverMedals / 5);
                const remainingSilver = silverMedals % 5;
                
                updates[`winnerLog/${date}_${p.name}`] = { 
                    date, 
                    name: p.name, 
                    cups: newValue,
                    silverMedals: remainingSilver,
                    goldMedals: goldMedals
                };
            }
            
            if (delta > 0) {
                if (side === 'left') {
                    const oldGoldCount = Math.floor(oldValue / 5);
                    const newGoldCount = Math.floor(newValue / 5);
                    if (newGoldCount > oldGoldCount) { goldMedalSound.play().catch(e => {}); }
                    else { silverMedalSound.play().catch(e => {}); }
                } else {
                    winSound.play().catch(e => {});
                    updates['lastWin'] = { timestamp: Date.now(), color: p.color || '#FFFFFF' };
                }
            }
            update(ref(db, `rooms/${roomId}`), updates);
        });
    }

    async function addPlayers() {
        const modal = document.getElementById('addPlayerModal');
        const input = document.getElementById('playerNamesInput');
        const addButton = document.getElementById('doAddPlayers');
        const cancelButton = document.getElementById('closeAddPlayerModal');

        input.value = '';
        openModal(modal);

        cancelButton.onclick = () => { closeModal(modal); };

        addButton.onclick = async () => {
            if (!roomId || !isAdmin) {
                alert("يجب أن تكون مشرفاً في غرفة لإضافة لاعب.");
                return;
            }

            const namesInput = input.value;
            if (!namesInput) { closeModal(modal); return; }

            const names = namesInput.trim().split(/[\s\n]+/).filter(name => name.length > 0);
            if (names.length === 0) { closeModal(modal); return; }

            const playersSnap = await get(ref(db, `rooms/${roomId}/players`));
            const existingPlayers = playersSnap.exists() ? playersSnap.val() : {};
            
            // جمع جميع الشخصيات المستخدمة حالياً
            let usedEmojis = Object.values(existingPlayers).map(p => p.character).filter(c => c);
            let usedColors = Object.values(existingPlayers).map(p => p.color).filter(c => c);

            for (const name of names) {
                // الحصول على شخصية غير مستخدمة
                const char = getAvailableCharacter(usedEmojis);
                usedEmojis.push(char); // إضافة للقائمة المستخدمة فوراً

                let availableColors = CARD_COLORS.filter(c => !usedColors.includes(c));
                if (availableColors.length === 0) {
                    // إذا انتهت الألوان، نبدأ من جديد
                    availableColors = [...CARD_COLORS];
                    usedColors = []; // إعادة تعيين الألوان المستخدمة
                }
                const color = availableColors[0];
                usedColors.push(color);

                await set(child(ref(db, `rooms/${roomId}/players`), makeId()), { name, left: 0, right: 0, character: char, color });
            }
            closeModal(modal);
        };
    }

    function initializeStaticEventListeners() {
        document.getElementById('addPlayerFab').addEventListener('click', () => { unlockSounds(); clickSound.play().catch(e => {}); addPlayers(); });
        document.getElementById('devInfoBtn').addEventListener('click', () => { unlockSounds(); clickSound.play().catch(e => {}); openModal(document.getElementById('devInfoModal')); });
        document.getElementById('refreshBtn').addEventListener('click', () => { unlockSounds(); clickSound.play().catch(e => {}); location.reload(); });
        document.getElementById("closeRoomModal").addEventListener('click', () => { clickSound.play().catch(e => {}); closeModal(document.getElementById('roomModal')); });
        document.getElementById("openLog").addEventListener('click', () => {
            clickSound.play().catch(e => {});
            closeModal(document.getElementById('roomModal'));
            openModal(document.getElementById('logPage'));
            
            // تحديث جدول السجل
            onValue(ref(db, `rooms/${roomId}/winnerLog`), (snap) => {
                const tbody = document.getElementById("logTableBody");
                if(!snap.exists()){ 
                    tbody.innerHTML = `<tr><td colspan="5" style="text-align: center; color: var(--muted); padding: 20px;">لا توجد سجلات بعد</td></tr>`; 
                    return; 
                }
                const list = Object.values(snap.val());
                list.sort((a,b)=> (b.date||'').localeCompare(a.date||''));
                tbody.innerHTML = list.map(e=>`
                    <tr>
                        <td>${e.date}</td>
                        <td>${e.name}</td>
                        <td>${e.cups||0}</td>
                        <td>${e.silverMedals||0}</td>
                        <td>${e.goldMedals||0}</td>
                    </tr>
                `).join("");
            }, { onlyOnce: true });
        });
        document.getElementById("closeLogPage").addEventListener('click', () => { clickSound.play().catch(e => {}); closeModal(document.getElementById('logPage')); });
        document.getElementById("clearLogBtn").addEventListener('click', () => { clickSound.play().catch(e => {}); openModal(document.getElementById('clearLogModal')); });
        document.getElementById("confirmClearLog").addEventListener('click', () => {
            clickSound.play().catch(e => {});
            if (roomId) {
                remove(ref(db, `rooms/${roomId}/winnerLog`));
            }
            closeModal(document.getElementById('clearLogModal'));
            closeModal(document.getElementById('logPage'));
        });
        document.getElementById("cancelClearLog").addEventListener('click', () => { clickSound.play().catch(e => {}); closeModal(document.getElementById('clearLogModal')); });
        document.getElementById("closeDeleteModal").addEventListener('click', () => { clickSound.play().catch(e => {}); closeModal(document.getElementById('deleteModal')); });
        document.getElementById("closeDevInfoModal").addEventListener('click', () => { clickSound.play().catch(e => {}); closeModal(document.getElementById('devInfoModal')); });
        document.getElementById('showJoinView').addEventListener('click', () => { clickSound.play().catch(e => {}); document.getElementById('roomManagementView').style.display = 'none'; document.getElementById('joinView').style.display = 'block'; });
        document.getElementById('backToManagement').addEventListener('click', () => { clickSound.play().catch(e => {}); document.getElementById('joinView').style.display = 'none'; document.getElementById('roomManagementView').style.display = 'block'; });
        document.getElementById("doCreate").addEventListener('click', async () => {
          clickSound.play().catch(e => {});
          const newRoomId = makeId(), newAdminKey = makeId();
          await set(ref(db, `rooms/${newRoomId}`), { createdAt: Date.now(), adminKey: newAdminKey });
          const adminTokens = JSON.parse(localStorage.getItem("uno-admin-tokens") || '{}');
          adminTokens[newRoomId] = newAdminKey;
          localStorage.setItem("uno-admin-tokens", JSON.stringify(adminTokens));
          history.replaceState({}, "", `${location.origin}${location.pathname}?room=${newRoomId}&admin=${newAdminKey}`);
          location.reload();
        });
        document.getElementById("doJoin").addEventListener('click', async () => {
          clickSound.play().catch(e => {});
          const joinRoomId = document.getElementById("joinId").value.trim();
          if(!joinRoomId){ alert("اكتب رمز الغرفة"); return; }
          const exists = (await get(child(ref(db, 'rooms'), joinRoomId))).exists();
          if(!exists){ alert("الغرفة غير موجودة"); return; }
          const adminTokens = JSON.parse(localStorage.getItem("uno-admin-tokens") || '{}');
          const newAdminToken = adminTokens[joinRoomId] || '';
          const url = new URL(location.href);
          url.searchParams.set('room', joinRoomId);
          if (newAdminToken) { url.searchParams.set('admin', newAdminToken); }
          else { url.searchParams.delete('admin'); }
          history.replaceState({}, "", url.toString());
          location.reload();
        });
        document.getElementById('copyLinkBtn').addEventListener('click', () => {
            clickSound.play().catch(e => {});
            const viewUrl = new URL(location.href);
            viewUrl.searchParams.delete('admin');
            navigator.clipboard.writeText(viewUrl.toString()).then(() => alert('تم نسخ رابط المشاهدة!'), () => alert('فشل نسخ الرابط.'));
        });
        document.getElementById('deletePlayerBtn').addEventListener('click', () => {
            clickSound.play().catch(e => {});
            if (currentTargetPlayerId) { remove(ref(db, `rooms/${roomId}/players/${currentTargetPlayerId}`)); }
            closeModal(document.getElementById('deleteModal'));
        });
        document.getElementById('deleteAllBtn').addEventListener('click', () => {
            clickSound.play().catch(e => {});
            if (confirm('تحذير: سيتم حذف جميع اللاعبين نهائياً. هل أنت متأكد؟')) {
                remove(ref(db, `rooms/${roomId}/players`));
            }
            closeModal(document.getElementById('deleteModal'));
        });
    }

    function initializeDynamicEventListeners() {
        let longPressTimer, isLongPress = false, touchEventDetected = false;
        
        // معالج الضغط على بطاقة الفائز
        function handleCardClick(e, container) {
            unlockSounds();
            
            // تجاهل الضغط إذا كان ضغط طويل
            if (isLongPress) {
                isLongPress = false;
                return;
            }
            
            const card = e.target.closest('.card, .winner-card');
            if (card && isAdmin) {
                const playerId = card.dataset.playerId;
                const rect = card.getBoundingClientRect();
                const clickY = e.clientY - rect.top;
                const clickX = e.clientX - rect.left;
                const cardHeight = rect.height;
                const cardWidth = rect.width;
                
                // تحديد المنطقة: نصف البطاقة العلوي للزيادة، النصف السفلي للتقليل
                const delta = clickY < cardHeight / 2 ? 1 : -1;
                
                // تحديد الجهة: يسار للميداليات، يمين للكؤوس
                let side;
                if (clickX < cardWidth / 2) {
                    side = 'right'; // الجهة اليسرى = الميداليات
                } else {
                    side = 'left'; // الجهة اليمنى = الكؤوس
                }
                
                handleScoreUpdate(playerId, side, delta);
                clickSound.play().catch(e => {});
            }
        }

        // معالج الضغط المطول لحذف اللاعب
        function handleCardPress(target, container) {
            if (!isAdmin) return;
            const card = target.closest('.card, .winner-card');
            if (card) {
                isLongPress = false;
                currentTargetPlayerId = card.dataset.playerId;
                longPressTimer = setTimeout(() => { 
                    isLongPress = true; 
                    openModal(document.getElementById('deleteModal')); 
                }, 1000);
            }
        }
        
        const handleRelease = () => {
            clearTimeout(longPressTimer);
        };

        // أحداث بطاقة الفائز
        winnerContainer.addEventListener('click', (e) => handleCardClick(e, winnerContainer));
        
        // أحداث بطاقات اللاعبين العاديين
        playersEl.addEventListener('click', (e) => handleCardClick(e, playersEl));

        // منع النافذة المنبثقة للصور فقط
        function preventContextMenu(e) {
            if (e.target.closest('.avatar')) {
                e.preventDefault();
                return false;
            }
        }

        winnerContainer.addEventListener('contextmenu', preventContextMenu);
        playersEl.addEventListener('contextmenu', preventContextMenu);

        // أحداث اللمس والماوس للحذف
        function setupPressEvents(container) {
            container.addEventListener('touchstart', (e) => { 
                touchEventDetected = true; 
                handleCardPress(e.target, container); 
            }, { passive: true });
            
            container.addEventListener('touchend', (e) => { 
                if (!touchEventDetected) return; 
                handleRelease(); 
            });
            
            container.addEventListener('mousedown', (e) => { 
                if (touchEventDetected || e.button !== 0) return; 
                handleCardPress(e.target, container); 
            });
            
            container.addEventListener('mouseup', (e) => { 
                if (touchEventDetected || e.button !== 0) return; 
                handleRelease(); 
            });

            const cancelPress = () => {
                clearTimeout(longPressTimer);
                isLongPress = false;
            };
            
            container.addEventListener('touchcancel', cancelPress);
            container.addEventListener('touchmove', cancelPress);
            container.addEventListener('mouseleave', cancelPress);
        }

        setupPressEvents(winnerContainer);
        setupPressEvents(playersEl);
    }

    (async function boot(){
      setRandomGradientBackground();
      initializeStaticEventListeners();
      await ensureAdminStatus();

      if(!roomId){
        renderHeader();
        document.getElementById('addPlayerFab').style.display = 'none';
        openModal(document.getElementById('roomModal'));
      } else {
        modalRoomId.textContent = roomId;
        localStorage.setItem("uno-room-id", roomId);

        onValue(ref(db, `rooms/${roomId}/players`), (snap) => {
            const playersData = snap.exists() ? snap.val() : {};
            const players = Object.entries(playersData).map(([id, v]) => ({ id, ...v }));
            players.sort((a, b) => (b.right || 0) - (a.right || 0) || (b.left || 0) - (a.left || 0));
            renderPlayers(players);
        });

        onValue(ref(db, `rooms/${roomId}/lastWin`), (snap) => {
            if (snap.exists()) {
                const winData = snap.val();
                triggerFireworks(winData); 
                if (isAdmin) {
                    remove(ref(db, `rooms/${roomId}/lastWin`));
                }
            }
        });
      }
      
      initializeDynamicEventListeners();
    })();
  </script>
</body>
</html><!DOCTYPE html>
<html lang="ar" dir="rtl">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>UNO Live — by Mr.K</title>
  <style>
    /* إعادة تعيين شاملة للمتصفحات */
    * { 
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }
    
    html, body { 
      font-size: 16px;
      line-height: 1.4;
      -webkit-font-smoothing: antialiased;
      -moz-osx-font-smoothing: grayscale;
    }
    
    :root {
      --bg:#0f1220; 
      --card-bg:#171a2b; 
      --text:#e8ecff; 
      --muted:#9aa3c7; 
      --accent:#7aa2ff;
      
      /* متغيرات موحدة ومتجاوبة */
      --base-font: clamp(14px, 2.5vw, 24px);
      --spacing: clamp(8px, 2vw, 16px);
      --border-radius: clamp(8px, 1.5vw, 16px);
      
      /* أحجام البطاقات */
      --card-min-height: clamp(110px, 20vh, 180px);
      --card-padding: var(--spacing);
      --card-gap: clamp(10px, 3vw, 20px);
      
      /* أحجام الأفتار */
      --avatar-size: clamp(70px, 15vw, 140px);
      --winner-avatar-size: clamp(100px, 18vw, 180px);
      
      /* أحجام النصوص */
      --score-font: clamp(18px, 4vw, 32px);
      --name-font: clamp(12px, 3vw, 22px);
      --icon-font: clamp(24px, 5vw, 48px);
      
      /* أحجام بطاقة الفائز */
      --winner-score-font: clamp(24px, 5vw, 48px);
      --winner-name-font: clamp(16px, 4vw, 32px);
      --winner-icon-font: clamp(30px, 6vw, 52px);
      --winner-card-height: clamp(140px, 25vh, 220px);
      
      /* أحجام الحشو */
      --score-padding: clamp(2px, 1vw, 6px) clamp(5px, 2vw, 12px);
      --name-padding: clamp(1px, 0.5vw, 4px) clamp(4px, 1.5vw, 8px);
    }
    
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      color: var(--text); 
      background-attachment: fixed; 
      transition: background 0.5s ease;
    }
    
    .wrap { 
      max-width: min(95vw, 1200px);
      margin: 0 auto; 
      padding: var(--spacing);
      padding-bottom: clamp(80px, 15vh, 120px);
    }
    
    /* الهيدر */
    .header {
      display: flex; 
      flex-direction: column; 
      align-items: center; 
      justify-content: center;
      gap: 0; 
      background: rgba(0, 0, 0, 0.35); 
      backdrop-filter: blur(10px);
      border: 1px solid rgba(255, 255, 255, 0.1); 
      padding: var(--spacing);
      border-radius: var(--border-radius); 
      box-shadow: 0 10px 30px rgba(0, 0, 0, .25);
      cursor: pointer; 
      position: relative; 
      width: 100%; 
      margin-bottom: var(--spacing);
    }
    
    .header .banner-img { 
      width: 100%; 
      height: clamp(80px, 15vh, 120px);
      object-fit: cover; 
      object-position: center; 
      border-radius: calc(var(--border-radius) * 0.5);
      animation: neonGlow 2s ease-in-out infinite alternate;
    }
    
    @keyframes neonGlow {
      0% {
        filter: brightness(1) drop-shadow(0 0 5px rgba(255,255,255,0.3));
      }
      50% {
        filter: brightness(1.2) drop-shadow(0 0 15px rgba(138,43,226,0.6)) drop-shadow(0 0 25px rgba(0,191,255,0.4));
      }
      100% {
        filter: brightness(1.4) drop-shadow(0 0 20px rgba(255,0,255,0.8)) drop-shadow(0 0 35px rgba(0,255,255,0.6));
      }
    }
    
    .title {
      position: absolute; 
      top: var(--spacing); 
      right: var(--spacing); 
      font-weight: 800; 
      letter-spacing: 0.5px;
      font-size: clamp(12px, 2.5vw, 16px);
      display: flex; 
      align-items: center; 
      gap: calc(var(--spacing) * 0.5);
      background: rgba(0,0,0,0.4); 
      padding: calc(var(--spacing) * 0.25) calc(var(--spacing) * 0.5);
      border-radius: calc(var(--border-radius) * 0.5);
    }
    
    .title .badge { 
      font-size: clamp(9px, 1.8vw, 11px);
      color: var(--text); 
      background: rgba(255,255,255,0.1); 
      padding: 2px 6px; 
      border-radius: 999px; 
    }

    /* بطاقة الفائز */
    .winner-section {
      width: 100%;
      margin-bottom: var(--spacing);
    }

    .winner-card {
      border-radius: var(--border-radius); 
      padding: calc(var(--spacing) * 1.25);
      box-shadow: 0 10px 40px rgba(0,0,0,0.4);
      position: relative; 
      user-select: none; 
      border: 2px solid rgba(255, 215, 0, 0.6);
      height: var(--winner-card-height);
      display: flex; 
      align-items: center;
      justify-content: center;
      width: 100%;
      background: linear-gradient(135deg, rgba(255, 215, 0, 0.15), rgba(255, 165, 0, 0.1));
      backdrop-filter: blur(15px);
      animation: winnerGlow 3s ease-in-out infinite alternate;
    }

    @keyframes winnerGlow {
      0% {
        box-shadow: 0 10px 40px rgba(0,0,0,0.4), 0 0 20px rgba(255, 215, 0, 0.3);
      }
      100% {
        box-shadow: 0 15px 50px rgba(0,0,0,0.5), 0 0 30px rgba(255, 215, 0, 0.6);
      }
    }

    .winner-card::before {
      content: "👑";
      position: absolute;
      top: clamp(-15px, -3vh, -5px);
      left: 50%;
      transform: translateX(-50%);
      font-size: clamp(40px, 8vw, 64px);
      z-index: 15;
      animation: crownFloat 2s ease-in-out infinite alternate;
    }

    @keyframes crownFloat {
      0% { transform: translateX(-50%) translateY(0px); }
      100% { transform: translateX(-50%) translateY(-5px); }
    }

    .winner-card .card-body {
      display: flex;
      align-items: center;
      justify-content: space-between;
      width: 100%;
      height: 100%;
      position: relative;
    }

    .winner-card .center-col { 
      text-align: center; 
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      position: absolute;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      z-index: 5;
    }

    .winner-card .avatar {
      width: var(--winner-avatar-size);
      height: var(--winner-avatar-size);
      object-fit: cover;
      background: transparent;
      -webkit-touch-callout: none;
      -webkit-user-select: none;
      pointer-events: none;
      border-radius: clamp(15px, 3vw, 25px);
      position: relative;
      z-index: 5;
      filter: drop-shadow(4px 8px 6px rgba(0,0,0,0.6));
      transform: translateY(clamp(-8px, -2vh, -20px));
      box-shadow: none;
      border: clamp(2px, 0.5vw, 4px) solid rgba(255,215,0,0.5);
    }
    
    /* الشبكة */
    .grid { 
      display: grid; 
      grid-template-columns: repeat(auto-fit, minmax(min(150px, 100%), 1fr));
      gap: var(--card-gap);
    }
    
    /* البطاقة الأساسية */
    .card {
      border-radius: var(--border-radius);
      padding: var(--card-padding);
      box-shadow: 0 6px 20px rgba(0,0,0,0.25);
      position: relative; 
      user-select: none; 
      border: 1px solid rgba(255, 255, 255, 0.1);
      min-height: var(--card-min-height);
      display: flex; 
      flex-direction: column; 
      align-items: center;
      justify-content: center;
    }
    
    .card-body {
      display: flex;
      align-items: center;
      justify-content: space-between;
      width: 100%;
      height: 100%;
      position: relative;
    }
    
    .center-col { 
      text-align: center; 
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      position: absolute;
      top: 50%;
      left: 50%;
      transform: translate(-50%, -50%);
      z-index: 5;
    }
    
    .avatar {
      width: var(--avatar-size);
      height: var(--avatar-size);
      object-fit: cover;
      background: transparent;
      -webkit-touch-callout: none;
      -webkit-user-select: none;
      pointer-events: none;
      border-radius: clamp(12px, 2.5vw, 20px);
      position: relative;
      z-index: 5;
      filter: drop-shadow(2px 4px 3px rgba(0,0,0,0.4));
      transform: translateY(0px);
      box-shadow: none;
    }

    /* حاوية العناصر السفلية */
    .card-header {
      position: absolute;
      bottom: calc(var(--spacing) * 0.5);
      left: 50%;
      transform: translateX(-50%);
      z-index: 10;
      pointer-events: none;
      display: flex;
      align-items: center;
      justify-content: space-between;
      width: 90%;
    }

    .winner-card .card-header {
      position: absolute;
      bottom: clamp(15px, 3vh, 25px);
      left: 50%;
      transform: translateX(-50%);
      z-index: 10;
      pointer-events: none;
      display: flex;
      align-items: center;
      justify-content: space-between;
      width: 90%;
    }

    /* منطقة النقاط */
    .score-area {
      display: flex; 
      flex-direction: row;
      align-items: center; 
      justify-content: center;
      gap: clamp(2px, 1vw, 4px);
      position: static;
      transform: none;
    }

    .score-area.left, .score-area.right {
      left: auto;
      right: auto;
      top: auto;
      transform: none;
    }

    /* الأيقونات العلوية */
    .top-icons {
      position: absolute;
      top: calc(var(--spacing) * 0.6);
      left: 50%;
      transform: translateX(-50%);
      display: flex;
      justify-content: space-between;
      width: clamp(95%, 98%, 100%);
      z-index: 15;
    }

    .winner-card .top-icons {
      top: clamp(10px, 2vh, 15px);
      width: clamp(95%, 98%, 100%);
    }

    .top-icon {
      font-size: var(--icon-font);
      opacity: 0.9;
      filter: drop-shadow(0 2px 4px rgba(0,0,0,0.5));
    }

    /* القاعدة الأساسية لجميع الأيقونات */
    .winner-card .top-icon {
      font-size: var(--winner-icon-font);
      opacity: 1;
      filter: drop-shadow(0 2px 4px rgba(0,0,0,0.5));
    }
    
    /* تكبير حجم الميدالية بنسبة 20% */
    .winner-card .top-icon.medal {
      font-size: calc(var(--winner-icon-font) * 1.5);
    }
    
    /* تكبير حجم الكأس بنسبة 30% */
    .winner-card .top-icon.trophy {
      font-size: calc(var(--winner-icon-font) * 1.5);
    }
    
    /* حاوية الاسم */
    .player-name-section {
      display: flex;
      align-items: center;
      font-weight: bold;
      font-size: var(--name-font);
      color: var(--text);
      padding: var(--name-padding);
      border-radius: calc(var(--border-radius) * 0.5);
      width: fit-content;
      background: linear-gradient(to left, rgba(255,255,255,0.3), rgba(255,255,255,0.05));
      backdrop-filter: blur(8px);
      border: 1px solid rgba(255,255,255,0.15);
      white-space: nowrap;
      max-width: clamp(80px, 25vw, 150px);
      overflow: hidden;
      text-overflow: ellipsis;
    }

    .winner-card .player-name-section {
      font-size: var(--winner-name-font);
      padding: clamp(3px, 1vw, 8px) clamp(6px, 2vw, 16px);
      border-radius: clamp(8px, 2vw, 20px);
      background: linear-gradient(to left, rgba(255,215,0,0.4), rgba(255,215,0,0.1));
      backdrop-filter: blur(10px);
      border: 2px solid rgba(255,215,0,0.3);
      text-shadow: 0 2px 4px rgba(0,0,0,0.5);
      max-width: clamp(100px, 30vw, 200px);
    }
    
    /* بطاقة النقاط */
    .score-card {
      background: linear-gradient(to top, rgba(255,255,255,0.25), rgba(255,255,255,0.05)); 
      backdrop-filter: blur(10px);
      border-radius: calc(var(--border-radius) * 0.4);
      padding: var(--score-padding);
      font-size: var(--score-font);
      font-weight: 900; 
      text-align: center; 
      min-width: clamp(28px, 6vw, 45px);
      border: 1px solid rgba(255,255,255,0.2); 
      box-shadow: 0 8px 32px rgba(0,0,0,0.1);
      transition: all 0.2s ease; 
      position: relative;
      pointer-events: none;
      color: var(--text);
      line-height: 1;
    }

    .winner-card .score-card {
      background: linear-gradient(to top, rgba(255,215,0,0.4), rgba(255,215,0,0.1)); 
      backdrop-filter: blur(15px);
      border-radius: clamp(6px, 1.5vw, 12px);
      padding: clamp(3px, 1vw, 10px) clamp(6px, 2vw, 18px);
      font-size: var(--winner-score-font);
      min-width: clamp(35px, 8vw, 70px);
      border: 2px solid rgba(255,215,0,0.4); 
      box-shadow: 0 8px 32px rgba(255,215,0,0.2);
      transition: all 0.3s ease; 
      text-shadow: 0 2px 4px rgba(0,0,0,0.5);
    }
    
    /* تحسينات التفاعل للشاشات الكبيرة */
    @media (hover: hover) and (pointer: fine) {
      .score-card:hover {
        background: rgba(255,255,255,0.35);
        transform: scale(1.05);
        border-color: rgba(255,255,255,0.5);
      }
      
      .btn:hover { 
        transform: translateY(-2px); 
      }
      
      .add-player-fab:hover { 
        transform: translateX(-50%) translateY(-2px); 
      }
      
      .corner-btn:hover { 
        transform: translateY(-2px); 
      }
    }
    
    /* تحسينات للشاشات عالية الدقة */
    @media screen and (-webkit-min-device-pixel-ratio: 2) {
      .avatar {
        image-rendering: -webkit-optimize-contrast;
        image-rendering: crisp-edges;
      }
      
      .score-card {
        font-weight: 800;
        letter-spacing: 0.5px;
      }
    }
    
    /* تحسينات لأجهزة اللمس */
    @media (hover: none) and (pointer: coarse) {
      .card, .winner-card {
        -webkit-tap-highlight-color: transparent;
      }
      
      .score-card {
        backface-visibility: hidden;
        -webkit-backface-visibility: hidden;
      }
    }
    
    /* باقي الأنماط الثابتة */
    .modal-backdrop{ 
      position: fixed; 
      inset: 0; 
      background: rgba(0,0,0,.45); 
      backdrop-filter: blur(5px); 
      display: none; 
      align-items: center; 
      justify-content: center; 
      z-index: 110; 
    }
    
    .modal{ 
      width: min(92vw, 560px); 
      background: #151935; 
      border: 1px solid #2a3257; 
      border-radius: var(--border-radius); 
      padding: var(--spacing); 
    }
    
    .modal h3{ 
      margin: 0 0 var(--spacing); 
      text-align: center; 
    }
    
    .input, input[type=text]{ 
      width: 100%; 
      background: #0f142e; 
      color: var(--text); 
      border: 1px solid #28325b; 
      border-radius: calc(var(--border-radius) * 0.75);
      padding: calc(var(--spacing) * 0.6) calc(var(--spacing) * 0.75);
      outline: none; 
      font-size: var(--base-font);
    }
    
    .row2{ 
      display: grid; 
      grid-template-columns: 1fr 1fr; 
      gap: calc(var(--spacing) * 0.6);
    }
    
    .room-info { 
      text-align: center; 
      margin-bottom: var(--spacing); 
      background: #0f142e; 
      padding: calc(var(--spacing) * 0.6);
      border-radius: calc(var(--border-radius) * 0.6);
      border: 1px dashed #2a3257; 
    }
    
    .room-info span { 
      font-weight: bold; 
      font-size: clamp(16px, 3vw, 18px);
      color: var(--accent); 
    }
    
    hr { 
      border-color: #2a32573d; 
      margin: var(--spacing) 0; 
    }
    
    .btn { 
      border-radius: 999px; 
      font-weight: bold; 
      border: none; 
      box-shadow: 0 4px 10px rgba(0,0,0,0.2); 
      transition: transform 0.2s ease; 
      cursor: pointer; 
      padding: calc(var(--spacing) * 0.6) calc(var(--spacing) * 0.9);
      font-size: clamp(12px, 2.5vw, 16px);
    }
    
    .add-player-fab, .corner-btn { 
      position: fixed; 
      bottom: clamp(15px, 3vh, 20px);
      border-radius: 999px; 
      border: none; 
      box-shadow: 0 4px 10px rgba(0,0,0,0.2); 
      cursor: pointer; 
      font-weight: bold; 
      transition: transform 0.2s ease; 
      z-index: 30;
      background: linear-gradient(to bottom, rgba(255,255,255,0.3), rgba(255,255,255,0.1));
      backdrop-filter: blur(10px);
      border: 1px solid rgba(255,255,255,0.2);
      color: white;
    }
    
    .add-player-fab { 
      left: 50%; 
      transform: translateX(-50%); 
      padding: clamp(10px, 2vh, 12px) clamp(20px, 4vw, 24px);
      font-size: clamp(16px, 3vw, 18px);
      display: flex; 
      align-items: center; 
      justify-content: center; 
      gap: calc(var(--spacing) * 0.5);
    }
    
    .corner-btn { 
      padding: calc(var(--spacing) * 0.6) clamp(15px, 3vw, 20px);
      font-size: clamp(12px, 2.5vw, 14px);
      display: grid; 
      place-items: center; 
    }
    
    #devInfoBtn { 
      left: clamp(15px, 3vw, 20px);
    }
    
    #refreshBtn { 
      right: clamp(15px, 3vw, 20px);
    }
    
    .info-item { 
      display: flex; 
      align-items: center; 
      gap: calc(var(--spacing) * 0.6);
      margin-bottom: calc(var(--spacing) * 0.75);
    }
    
    .info-item a { 
      color: var(--text); 
      text-decoration: none; 
    }
    
    .info-item a:hover { 
      color: var(--accent); 
    }
    
    /* صفحة السجل */
    .log-page {
      width: 95vw;
      height: 95vh;
      background: var(--card-bg);
      border: 1px solid rgba(255,255,255,0.1);
      border-radius: var(--border-radius);
      padding: clamp(16px, 3vw, 24px);
      display: flex;
      flex-direction: column;
      overflow: hidden;
    }
    
    .log-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: clamp(16px, 3vh, 24px);
      padding-bottom: var(--spacing);
      border-bottom: 2px solid rgba(255,255,255,0.1);
    }
    
    .log-header h2 {
      margin: 0;
      color: var(--text);
      font-size: clamp(20px, 4vw, 24px);
    }
    
    .log-actions {
      display: flex;
      gap: calc(var(--spacing) * 0.75);
    }
    
    .log-btn {
      background: linear-gradient(to bottom, rgba(255,255,255,0.2), rgba(255,255,255,0.05));
      backdrop-filter: blur(10px);
      border: 1px solid rgba(255,255,255,0.2);
      color: var(--text);
      padding: calc(var(--spacing) * 0.5) var(--spacing);
      border-radius: calc(var(--border-radius) * 0.5);
      font-size: clamp(12px, 2.5vw, 14px);
    }
    
    .log-content {
      flex: 1;
      overflow: auto;
    }
    
    .log-table {
      width: 100%;
      border-collapse: collapse;
      font-size: clamp(14px, 3vw, 16px);
    }
    
    .log-table th {
      background: linear-gradient(to bottom, rgba(255,255,255,0.2), rgba(255,255,255,0.05));
      padding: calc(var(--spacing) * 0.75);
      text-align: center;
      border: 1px solid rgba(255,255,255,0.1);
      color: var(--text);
      font-weight: bold;
    }
    
    .log-table td {
      padding: calc(var(--spacing) * 0.6);
      text-align: center;
      border: 1px solid rgba(255,255,255,0.1);
      color: var(--text);
      background: rgba(255,255,255,0.05);
    }
    
    .log-table tr:nth-child(even) td {
      background: rgba(255,255,255,0.08);
    }
    
    .log-footer {
      display: flex;
      justify-content: space-between;
      margin-top: var(--spacing);
      gap: calc(var(--spacing) * 0.75);
    }
    
    .log-footer-btn {
      flex: 1;
      padding: calc(var(--spacing) * 0.75);
      font-size: clamp(14px, 3vw, 16px);
    }
    
    #confetti-canvas, #screen-flash { 
      position: fixed; 
      top: 0; 
      left: 0; 
      width: 100%; 
      height: 100%; 
      pointer-events: none; 
      visibility: hidden; 
      opacity: 0; 
    }
    
    #confetti-canvas { 
      z-index: 120; 
      transition: opacity 0.3s; 
    }
    
    #screen-flash { 
      z-index: 121; 
      background-color: white; 
      transition: opacity 0.5s ease-out; 
    }
    
    #confetti-canvas.active, #screen-flash.active { 
      visibility: visible; 
      opacity: 1; 
    }
    
    #screen-flash.active { 
      opacity: 0.7; 
    }
    
    #playerNamesInput {
      resize: vertical;
      font-size: var(--base-font);
      text-align: center;
    }
    
    /* تحسين الشبكة للشاشات الكبيرة جداً */
    @media screen and (min-width: 1200px) {
      .grid {
        grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
      }
    }
    
    /* تحسين للشاشات العريضة جداً */
    @media screen and (min-width: 1600px) {
      .wrap {
        max-width: 1400px;
      }
      
      .grid {
        grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
      }
    }
  </style>
</head>
<body>
  <canvas id="confetti-canvas"></canvas>
  <div id="screen-flash"></div>
  <div class="wrap">
    <div id="header-container"></div>
    <div id="winner-container"></div>
    <div class="grid" id="players"></div>
  </div>

  <button class="add-player-fab btn" id="addPlayerFab"><span>+</span> إضافة لاعب</button>
  <button class="corner-btn btn" id="devInfoBtn">MR.K</button>
  <button class="corner-btn btn" id="refreshBtn">تحديث</button>

  <!-- Modals -->
  <div class="modal-backdrop" id="roomModal"><div class="modal"><div id="roomManagementView"><h3>إدارة الغرفة</h3><div class="room-info" id="roomInfoBlock">رمز الغرفة الحالي: <span id="modalRoomId"></span></div><div class="row2" style="margin-bottom: 10px;"><button class="btn" id="openLog">📜 عرض السجل</button><button class="btn" id="copyLinkBtn">🔗 نسخ رابط المشاهدة</button></div><hr><div class="row2"><button class="btn" id="doCreate">🚀 إنشاء غرفة جديدة</button><button class="btn" id="showJoinView">🚪 الانضمام لغرفة أخ
