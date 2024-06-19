/* Game starts with some generic boilerplate that can probably stay the same for any game.
 */
 
import { BTN_LEFT, BTN_RIGHT, BTN_UP, BTN_DOWN, BTN_A, BTN_B, BTN_START } from "./Input.js";
import { SCREENW, SCREENH } from "./constants.js";

export class Game {
  constructor(video, audio, input) {
    this.video = video;
    this.audio = audio;
    this.input = input;
    video.game = this;
    this.running = false;
    this.pendingFrame = null;
    this.pvtime = 0;
    this.input.keyHandlers.Escape = () => this.end();
  }

  begin() {
    if (this.running) return;
    this.running = true;
    this.resetModel();
    this.video.render();
    this.pvtime = Date.now();
    this.pendingFrame = window.requestAnimationFrame(() => this.update());
  }
  
  end() {
    this.video.end();
    this.audio.end();
    this.input.end();
    this.running = false;
    if (this.pendingFrame) {
      window.cancelAnimationFrame(this.pendingFrame);
      this.pendingFrame = null;
    }
  }
  
  update() {
    this.pendingFrame = null;
    if (!this.running) return;
    this.audio.update();
    const state = this.input.update();
    const now = Date.now();
    let elapsed = now - this.pvtime;
    if (elapsed >= 10) { // Wait until sufficient time has passed. Maximum 100 Hz.
      if (elapsed > 20) { // Updating slower than 50 Hz, clamp it and run slow.
        elapsed = 20;
      }
      this.updateModel(elapsed / 1000, state);
      this.pvtime = now;
    }
    this.video.render();
    this.pendingFrame = window.requestAnimationFrame(() => this.update());
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
  }
  
  updateModel(elapseds, state) {
    if (state !== this.pvstate) {
      console.log(`INPUT 0x${state.toString(16)}`);
      if ((state & BTN_LEFT) && !(this.pvstate & BTN_LEFT)) this.herodx = -1;
      else if (!(state & BTN_LEFT) && (this.pvstate & BTN_LEFT) && (this.herodx < 0)) this.herodx = 0;
      if ((state & BTN_RIGHT) && !(this.pvstate & BTN_RIGHT)) this.herodx = 1;
      else if (!(state & BTN_RIGHT) && (this.pvstate & BTN_RIGHT) && (this.herodx > 0)) this.herodx = 0;
      if ((state & BTN_UP) && !(this.pvstate & BTN_UP)) this.herody = -1;
      else if (!(state & BTN_UP) && (this.pvstate & BTN_UP) && (this.herody < 0)) this.herody = 0;
      if ((state & BTN_DOWN) && !(this.pvstate & BTN_DOWN)) this.herody = 1;
      else if (!(state & BTN_DOWN) && (this.pvstate & BTN_DOWN) && (this.herody > 0)) this.herody = 0;
      this.pvstate = state;
    }
    if (this.herodx || this.herody) {
      const speed = 100; // px/s
      this.herox += this.herodx * elapseds * speed;
      this.heroy += this.herody * elapseds * speed;
    }
  }
}
