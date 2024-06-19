import { SCREENW, SCREENH } from "./constants.js";

export class Video {
  constructor() {
    this.canvas = window.document.querySelector("canvas");
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
  }

}
