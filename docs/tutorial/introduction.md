import { useState, useEffect, useRef, useCallback } from "react";
import { motion } from "framer-motion";
import { Settings2, Info, Play, Square, Zap } from "lucide-react";
import KeyCapture from "../components/KeyCapture";
import CpsSlider from "../components/CpsSlider";

export default function Macro() {
  const [activationKey, setActivationKey] = useState("F6");
  const [spamKey, setSpamKey] = useState("e");
  const [cps, setCps] = useState(10);
  const [active, setActive] = useState(false);
  const [totalClicks, setTotalClicks] = useState(0);
  const workerRef = useRef(null);
  const clickCountRef = useRef(0);
  const spamKeyRef = useRef(spamKey);
  spamKeyRef.current = spamKey;

  useEffect(() => {
    const handleKeyDown = (e) => {
      const key = e.key === " " ? "Space" : e.key;
      if (key === activationKey) {
        e.preventDefault();
        setActive((prev) => !prev);
      }
    };
    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, [activationKey]);

  // Init worker once
  useEffect(() => {
    workerRef.current = new Worker("/macro-worker.js");
    workerRef.current.onmessage = () => {
      const key = spamKeyRef.current;
      const keyValue = key === "Space" ? " " : key;
      if (key.startsWith("Mouse")) {
        const btn = key === "Mouse4" ? 3 : 4;
        document.dispatchEvent(new MouseEvent("mousedown", { button: btn, bubbles: true }));
        document.dispatchEvent(new MouseEvent("mouseup", { button: btn, bubbles: true }));
      } else {
        document.dispatchEvent(new KeyboardEvent("keydown", { key: keyValue, bubbles: true }));
        document.dispatchEvent(new KeyboardEvent("keyup", { key: keyValue, bubbles: true }));
      }
      clickCountRef.current += 1;
      if (clickCountRef.current % 10 === 0) setTotalClicks(clickCountRef.current);
    };
    return () => workerRef.current?.terminate();
  }, []);

  useEffect(() => {
    if (!workerRef.current) return;
    if (active) {
      workerRef.current.postMessage({ type: "start", cps });
    } else {
      workerRef.current.postMessage({ type: "stop" });
    }
  }, [active, cps]);

  const handleToggle = useCallback(() => setActive((prev) => !prev), []);

  return (
    <div className="min-h-screen bg-background flex items-center justify-center p-3">
      <motion.div
        initial={{ opacity: 0, y: 16 }}
        animate={{ opacity: 1, y: 0 }}
        transition={{ duration: 0.4 }}
        className="w-full max-w-xs"
      >
        {/* Header */}
        <div className="text-center mb-4">
          <div className="inline-flex items-center gap-2">
            <div className="w-1.5 h-1.5 rounded-full bg-primary" />
            <h1 className="text-xl font-bold tracking-widest font-rajdhani uppercase">
              Zequ Macro
            </h1>
            <div className="w-1.5 h-1.5 rounded-full bg-accent" />
          </div>
          <p className="text-xs text-muted-foreground font-mono mt-0.5">key spammer</p>
        </div>

        {/* Card */}
        <div className="bg-card border border-border rounded-xl overflow-hidden">
          {/* Status row */}
          <div className="flex items-center justify-between px-4 py-3 border-b border-border">
            <div className="flex items-center gap-2">
              <div className={`w-2 h-2 rounded-full transition-colors duration-300 ${active ? "bg-primary animate-pulse" : "bg-muted-foreground/40"}`} />
              <span className={`text-xs font-semibold uppercase tracking-widest font-rajdhani ${active ? "text-primary" : "text-muted-foreground"}`}>
                {active ? "Running" : "Idle"}
              </span>
            </div>
            <div className="flex items-center gap-1.5 text-xs text-muted-foreground font-mono">
              <Zap className="w-3 h-3" />
              <span>{totalClicks.toLocaleString()}</span>
            </div>
          </div>

          {/* Toggle */}
          <div className="px-4 py-3">
            <button
              onClick={handleToggle}
              className={`w-full py-2.5 rounded-lg font-bold text-xs uppercase tracking-widest font-rajdhani transition-all duration-300 flex items-center justify-center gap-2
                ${active
                  ? "bg-destructive/20 border border-destructive text-destructive hover:bg-destructive/30"
                  : "bg-primary/20 border border-primary text-primary hover:bg-primary/30"
                }`}
            >
              {active ? <Square className="w-3.5 h-3.5" /> : <Play className="w-3.5 h-3.5" />}
              {active ? "Stop" : "Start"}
            </button>
          </div>

          <div className="h-px bg-border" />

          {/* Settings */}
          <div className="p-4 space-y-4">
            <div className="flex items-center gap-1.5 mb-1">
              <Settings2 className="w-3.5 h-3.5 text-muted-foreground" />
              <span className="text-xs font-semibold uppercase tracking-widest text-muted-foreground font-rajdhani">Config</span>
            </div>

            <KeyCapture value={activationKey} onChange={setActivationKey} label="Activation Key" />
            <KeyCapture value={spamKey} onChange={setSpamKey} label="Spam Key" allowMouse />
            <CpsSlider value={cps} onChange={setCps} />
          </div>

          {/* Info */}
          <div className="px-4 pb-4">
            <div className="flex items-start gap-2 bg-secondary/50 rounded-lg p-2.5 border border-border">
              <Info className="w-3.5 h-3.5 text-muted-foreground shrink-0 mt-0.5" />
              <p className="text-xs text-muted-foreground leading-relaxed font-mono">
                <span className="text-foreground bg-secondary px-1 rounded">{activationKey}</span> toggles &bull; spam: <span className="text-foreground bg-secondary px-1 rounded">{spamKey}</span> &bull; <span className="text-primary">{cps} CPS</span>
              </p>
            </div>
          </div>
        </div>

        <p className="text-center text-xs text-muted-foreground/30 mt-4 font-mono">zequ v1.0</p>
      </motion.div>
    </div>
  );
}
