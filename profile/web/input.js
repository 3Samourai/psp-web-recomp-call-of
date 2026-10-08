/* Browser inputs -> PSP buttons and the single PSP analog stick. */
(function (root) {
  const PSP_KEYS = {ArrowUp:16, ArrowRight:32, ArrowDown:64, ArrowLeft:128,
    KeyZ:16384, KeyX:8192, KeyA:32768, KeyS:4096, KeyQ:256, KeyW:512,
    Enter:8, ShiftLeft:1, ShiftRight:1};
  const FPS_KEYS = {ArrowUp:16, ArrowRight:32, ArrowDown:64, ArrowLeft:128,
    KeyZ:16384, KeyX:8192, Enter:16384, Escape:8, Space:1,
    KeyE:16, KeyR:128, KeyC:64, Digit1:32,
    KeyQ:256, ShiftLeft:256, ShiftRight:256, KeyV:512, ControlLeft:512, ControlRight:512,
    KeyI:4096, KeyK:16384, KeyJ:32768, KeyL:8192};
  const PSP_STICK = {KeyJ:[-1,0], KeyL:[1,0], KeyI:[0,-1], KeyK:[0,1]};
  const FPS_STICK = {KeyA:[-1,0], KeyD:[1,0], KeyW:[0,-1], KeyS:[0,1]};
  const PAD = [16384,8192,32768,4096,256,512,256,512,1,8,1,16,16,64,128,32];
  const clamp = x => Math.max(-1, Math.min(1, Number.isFinite(x) ? x : 0));
  const deadzone = x => Math.abs(x || 0) <= 0.18 ? 0 : Math.sign(x) * (Math.abs(x) - 0.18) / 0.82;
  class PspInput {
    constructor(profile = 'psp') { this.profile = profile; this.clear(); }
    clear() { this.keys = new Set(); }
    setProfile(profile) { this.profile = profile === 'fps' ? 'fps' : 'psp'; this.clear(); }
    recognizes(code) { return code in this.buttonMap() || code in this.stickMap(); }
    buttonMap() { return this.profile === 'fps' ? FPS_KEYS : PSP_KEYS; }
    stickMap() { return this.profile === 'fps' ? FPS_STICK : PSP_STICK; }
    key(code, down) { if (down) this.keys.add(code); else this.keys.delete(code); }
    sample(pads = [], touch = {}) {
      let buttons = touch.buttons || 0, ax = 0, ay = 0;
      for (const code of this.keys) {
        buttons |= this.buttonMap()[code] || 0;
        const direction = this.stickMap()[code];
        if (direction) { ax += direction[0]; ay += direction[1]; }
      }
      if (touch.stick) { ax = touch.x; ay = touch.y; }
      for (const pad of pads) {
        if (!pad || pad.connected === false) continue;
        pad.buttons.forEach((button, n) => {
          if ((n === 10 || n === 11) && this.profile !== 'fps') return;
          if (button.pressed || button.value > 0.5) buttons |= PAD[n] || 0;
        });
        const x = deadzone(pad.axes[0]), y = deadzone(pad.axes[1]);
        if (x) ax = x;
        if (y) ay = y;
        if (this.profile === 'fps') {
          if (pad.axes[2] < -0.3) buttons |= 32768;
          if (pad.axes[2] > 0.3) buttons |= 8192;
          if (pad.axes[3] < -0.3) buttons |= 4096;
          if (pad.axes[3] > 0.3) buttons |= 16384;
        }
      }
      const length = Math.hypot(ax, ay);
      if (length > 1) { ax /= length; ay /= length; }
      return {buttons, lx:Math.round(128 + clamp(ax) * 127), ly:Math.round(128 + clamp(ay) * 127)};
    }
  }
  if (typeof module === 'object' && module.exports) module.exports = PspInput;
  else root.PspInput = PspInput;
})(typeof window === 'object' ? window : globalThis);
