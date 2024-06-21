/**
 * This file is loaded via the <script> tag in the index.html file and will
 * be executed in the renderer process for that window. No Node.js APIs are
 * available in this process because `nodeIntegration` is turned off and
 * `contextIsolation` is turned on. Use the contextBridge API in `preload.js`
 * to expose Node.js functionality from the main process.
 */
const body = document.body;
const leftSidebar = document.getElementById('left-sidebar');
const leftSidebarButton = document.getElementById("toggle-left-sidebr-button");
const rightSidebar = document.getElementById('right-sidebar');
const rightSidebarButton = document.getElementById("toggle-right-sidebr-button");

const content = document.getElementById("content");

let leftOpen = false;
let rightOpen = false;

leftSidebarButton.addEventListener('click', () => {
    // Fix content size while animation is happening
    content.style.width = `${content.clientWidth}px`;
    body.style.grid = '1fr / 1fr auto auto';
    leftSidebar.style.width = 'auto';
    setTimeout(() => {
        content.style.width = 'auto';
        body.style.grid = '1fr / auto 1fr auto';
        leftSidebar.style.width = leftOpen ? '200px': '0';
    }, 500);


    window.electronAPI.toggleLeftSidebar();
    leftOpen = !leftOpen;
});

rightSidebarButton.addEventListener('click', () => {
    // Fix content size while animation is happening
    content.style.width = `${content.clientWidth}px`;
    body.style.grid = '1fr / auto auto 1fr';
    rightSidebar.style.width = 'auto';
    setTimeout(() => {
        content.style.width = 'auto';
        body.style.grid = '1fr / auto 1fr auto';
        rightSidebar.style.width = rightOpen ? '200px': '0';

    }, 500);

    window.electronAPI.toggleRightSidebar();
    rightOpen = !rightOpen;
});
