from asyncio import get_event_loop

from terminal import Terminal, Keyboard

loop = get_event_loop()


class DiffApp:
    def __init__(self):
        self.terminal = Terminal()
        self.keyboard = Keyboard()
        self.loop = get_event_loop()

    def _read_stdin(self):
        print(self.keyboard[self.terminal.infile.read()])

    def run(self):
        if self.terminal.infile.isatty():
            self.loop.add_reader(self.terminal.infile, self._read_stdin)
        with self.terminal.unbuffered_input(), (
                self.terminal.nonblocking_input()):
            self.loop.run_forever()


if __name__ == '__main__':
    DiffApp().run()
