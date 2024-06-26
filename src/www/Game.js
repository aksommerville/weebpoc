/* Game starts with some generic boilerplate that can probably stay the same for any game.
 */
 
import { BTN_LEFT, BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_A, BTN_B, BTN_START } from "./Input.js";
import { SCREENW, SCREENH } from "./constants.js";
import { SND_PEW, SND_BLIP, SND_NNRGH } from "./Audio.js";

export class Game {
  constructor(v, a, i) {
    this.v = v;
    this.a = a;
    this.i = i;
    v.game = this;
    this.running = false;
    this.pf = null; // Pending Frame
    this.pvtime = 0;
    this.i.kh.Escape = () => this.end();
  }

  begin() {
    if (this.running) return;
    this.running = true;
    this.resetModel();
    this.v.render();
    this.pvtime = Date.now();
    this.pf = window.requestAnimationFrame(() => this.update());
  }
  
  end() {
    this.v.end();
    this.a.end();
    this.i.end();
    this.running = false;
    if (this.pf) {
      window.cancelAnimationFrame(this.pf);
      this.pf = null;
    }
  }
  
  update() {
    this.pf = null;
    if (!this.running) return;
    this.a.update();
    const state = this.i.update();
    const now = Date.now();
    let elapsed = now - this.pvtime;
    if (elapsed >= 10) { // Wait until sufficient time has passed. Maximum 100 Hz.
      if (elapsed > 20) { // Updating slower than 50 Hz, clamp it and run slow.
        elapsed = 20;
      }
      this.updateModel(elapsed / 1000, state);
      this.pvtime = now;
    }
    this.v.render();
    this.pf = window.requestAnimationFrame(() => this.update());
  }
  
  /* End of generic game manager.
   * Everything below this point should be specific to this game.
   ************************************************************************/
   
  resetModel() {
    this.herox = SCREENW / 2;
    this.heroy = SCREENH / 2;
    this.herodx = 0;
    this.herody = 0;
    this.pvstate = 0;
    this.a.playSong("eye_of_newt");
  }
  
  updateModel(elapseds, state) {
    if (state !== this.pvstate) {
      //console.log(`INPUT 0x${state.toString(16)}`);
      const dn = b => ((state&b) && !(this.pvstate&b));
      const up = b => (!(state&b) && (this.pvstate&b));
      if (dn(BTN_LEFT)) this.herodx = -1; else if (up(BTN_LEFT) && (this.herodx < 0)) this.herodx = 0;
      if (dn(BTN_RIGHT)) this.herodx = 1; else if (up(BTN_RIGHT) && (this.herodx > 0)) this.herodx = 0;
      if (dn(BTN_UP)) this.herody = -1; else if (up(BTN_UP) && (this.herody < 0)) this.herody = 0;
      if (dn(BTN_DOWN)) this.herody = 1; else if (up(BTN_DOWN) && (this.herody > 0)) this.herody = 0;
      if (dn(BTN_A)) this.a.snd(SND_PEW);
      if (dn(BTN_B)) this.a.snd(SND_BLIP);
      if (dn(BTN_START)) this.a.snd(SND_NNRGH);
      this.pvstate = state;
    }
    if (this.herodx || this.herody) {
      const speed = 100; // px/s
      this.herox += this.herodx * elapseds * speed;
      this.heroy += this.herody * elapseds * speed;
    }
  }
}
