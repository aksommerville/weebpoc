import { SCREENW, SCREENH } from "./constants.js";
import { Video } from "./Video.js";
import { Input } from "./Input.js";
import { Audio } from "./Audio.js";
import { Game } from "./Game.js";

window.addEventListener("load", () => {
  const video = new Video();
  const audio = new Audio();
  const input = new Input();
  const game = new Game(video, audio, input);
  game.begin();
});
