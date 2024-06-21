/* Game starts with some generic boilerplate that can probably stay the same for any game.
 */
 
import { BTN_LEFT, BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_A, BTN_B, BTN_START } from "./Input.js";
import { SCREENW, SCREENH } from "./constants.js";
//import { SND_PEW, SND_BLIP, SND_NNRGH } from "./Audio.js";

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
    //TODO
    //this.pvstate = 0;
  }
  
  updateModel(elapseds, state) {
    /*TODO
    if (state !== this.pvstate) {
      const dn = b => ((state&b) && !(this.pvstate&b));
      if (dn(BTN_A)) this.a.snd(SND_PEW);
      if (dn(BTN_B)) this.a.snd(SND_BLIP);
      if (dn(BTN_START)) this.a.snd(SND_NNRGH);
      this.pvstate = state;
    }
    /**/
  }
}
