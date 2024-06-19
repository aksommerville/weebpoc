import { SCREENW, SCREENH } from "./constants.js";

export class Video {
  constructor() {
    this.srcbits = document.querySelector("img"); // If the game needs multiple images, you might need to adjust the HTML minifier.
    this.canvas = document.querySelector("canvas");
    this.canvas.width = SCREENW;
    this.canvas.height = SCREENH;
    this.ctx = this.canvas.getContext("2d");
  }

  render() {
    this.ctx.fillStyle = "#00f";
    this.ctx.fillRect(0, 0, SCREENW, SCREENH);
    this.ctx.beginPath();
    this.ctx.moveTo(0, 0);
    this.ctx.lineTo(SCREENW, SCREENH);
    this.ctx.strokeStyle = "#fff";
    this.ctx.stroke();
    this.ctx.drawImage(this.srcbits, 12, 8, 48, 42, 100, 100, 48, 42);
  }

}
