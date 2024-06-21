import { SCREENW, SCREENH } from "./constants.js";
import { Video } from "./Video.js";
import { Input } from "./Input.js";
import { Audio } from "./Audio.js";
import { Game } from "./Game.js";

window.addEventListener("load", () => {
  const game = new Game(new Video(), new Audio(), new Input());
  game.begin();
});
