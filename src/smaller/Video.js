import { SCREENW, SCREENH } from "./constants.js";
import { BTN_A, BTN_B, BTN_START } from "./Input.js";

export class Video {
  constructor() {
    this.srcbits = document.querySelector("img"); // If the game needs multiple images, you might need to adjust the HTML minifier.
    this.canvas = document.querySelector("canvas");
    this.canvas.width = SCREENW;
    this.canvas.height = SCREENH;
    this.ctx = this.canvas.getContext("2d");
  }
  
  end() {
    this.ctx.fillStyle = "#888";
    this.ctx.fillRect(0, 0, SCREENW, SCREENH);
  }
  
  // Highly stubful... you know what to do
  // Game assigns itself to Video at construction. (this.game) is guaranteed available during render().

  render() {
    this.ctx.fillStyle = "#48f";
    this.ctx.fillRect(0, 0, SCREENW, SCREENH);
  }

}
