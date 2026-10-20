import type { WebViewImplHooks } from '@electron/internal/renderer/web-view/web-view-impl';
import type * as internal from '@electron/internal/renderer/web-view/webview2-internal';
export interface WebView2Hooks {
  webview2Internal: typeof internal;
  allowGuestViewElementDefinition: WebViewImplHooks['allowGuestViewElementDefinition'];
  setIsWebView: WebViewImplHooks['setIsWebView'];
}
let nextId = 0;
export function setupWebView2(hooks: WebView2Hooks) {
  const register = () => {
    if (customElements.get('webview2')) return;
    const states = new WeakMap<HTMLElement, State>();
    class State {
      readonly id = ++nextId;
      generation = 0;
      slot?: string;
      receipt?: internal.Receipt;
      unsubscribe?: () => void;
      readonly root: ShadowRoot;
      constructor(readonly element: HTMLElement) {
        this.root = element.attachShadow({ mode: 'closed' });
        const style = document.createElement('style');
        style.textContent =
          ':host{display:flex}iframe{width:100%;height:100%;flex:1;border:0;min-width:0;min-height:0}';
        this.root.append(style);
      }
      emit(name: string, receipt: internal.Receipt) {
        this.element.dispatchEvent(new CustomEvent(name, { detail: receipt }));
      }
      clearFrame() {
        this.root.querySelector('iframe')?.remove();
      }
      stop() {
        ++this.generation;
        const slot = this.slot;
        const receipt = this.receipt;
        this.slot = undefined;
        this.receipt = undefined;
        this.unsubscribe?.();
        this.unsubscribe = undefined;
        const released = slot ? hooks.webview2Internal.release(slot) : null;
        this.clearFrame();
        if (receipt || released) this.emit('detached', receipt || released!);
      }
      start() {
        if (!this.element.isConnected || this.slot) return;
        const value = this.element.getAttribute('web-contents-id');
        if (!value || !/^[1-9][0-9]*$/.test(value) || !Number.isSafeInteger(Number(value))) return;
        const generation = ++this.generation;
        const slot = `${this.id}:${generation}`;
        this.slot = slot;
        const iframe = document.createElement('iframe');
        hooks.setIsWebView(iframe);
        this.unsubscribe = hooks.webview2Internal.subscribe((incomingSlot, receipt) => {
          if (this.slot !== incomingSlot || this.receipt?.attachmentId !== receipt.attachmentId) return;
          this.receipt = undefined;
          this.slot = undefined;
          hooks.webview2Internal.release(incomingSlot);
          this.unsubscribe?.();
          this.unsubscribe = undefined;
          this.clearFrame();
          this.emit('detached', receipt);
        });
        this.root.append(iframe);
        try {
          const receipt = hooks.webview2Internal.attach(iframe, slot, Number(value));
          this.receipt = receipt;
          this.emit('attached', receipt);
        } catch (error) {
          this.stop();
          console.error('webview2 attachment failed:', error);
        }
      }
    }
    class WebView2 extends HTMLElement {
      static get observedAttributes() {
        return ['web-contents-id'];
      }
      constructor() {
        super();
        states.set(this, new State(this));
      }
      connectedCallback() {
        states.get(this)!.start();
      }
      disconnectedCallback() {
        states.get(this)!.stop();
      }
      attributeChangedCallback(_name: string, oldValue: string | null, value: string | null) {
        if (oldValue === value) return;
        const state = states.get(this)!;
        state.stop();
        state.start();
      }
    }
    hooks.allowGuestViewElementDefinition(window, () => {
      customElements.define('webview2', WebView2);
      delete (WebView2.prototype as any).connectedCallback;
      delete (WebView2.prototype as any).disconnectedCallback;
      delete (WebView2.prototype as any).attributeChangedCallback;
      delete (WebView2 as any).observedAttributes;
    });
  };
  if (document.readyState !== 'loading') register();
  else document.addEventListener('DOMContentLoaded', register, { once: true });
}
