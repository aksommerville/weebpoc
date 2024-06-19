export class Game {
  constructor(video, audio, input) {
    this.video = video;
    this.audio = audio;
    this.input = input;
  }

  begin() {
    this.video.render();
  }
}
