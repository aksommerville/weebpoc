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

  render() {
    this.ctx.fillStyle = "#48f";
    this.ctx.fillRect(0, 0, SCREENW, SCREENH);
    this.ctx.beginPath();
    this.ctx.moveTo(0, 0);
    this.ctx.lineTo(SCREENW, SCREENH);
    this.ctx.strokeStyle = "#fff";
    this.ctx.stroke();
    const dstx = Math.round(this.game.herox - 12);
    const dsty = Math.round(this.game.heroy - 12);
    this.ctx.drawImage(this.srcbits, 76, 17, 23, 24, dstx, dsty, 23, 24);
    if (this.game.pvstate & BTN_B) this.ctx.drawImage(this.srcbits, 80, 41, 6, 7, dstx + 2, dsty + 11, 6, 7);
    else this.ctx.drawImage(this.srcbits, 76, 41, 4, 6, dstx + 3, dsty + 15, 4, 6);
    if (this.game.pvstate & BTN_A) this.ctx.drawImage(this.srcbits, 90, 41, 8, 7, dstx + 15, dsty + 11, 8, 7);
    else this.ctx.drawImage(this.srcbits, 86, 41, 4, 7, dstx + 16, dsty + 14, 4, 7);
    if (this.game.pvstate & BTN_START) this.ctx.drawImage(this.srcbits, 76, 48, 5, 5, dstx + 9, dsty + 10, 5, 5);
  }

}
