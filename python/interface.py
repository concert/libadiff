from asyncio import get_event_loop
import sys
import argh

from terminal import Terminal, Keyboard

loop = get_event_loop()


class DiffApp:
    def __init__(self, filename_a, filename_b):
        self.filename_a = filename_a
        self.filename_b = filename_b
        self.terminal = Terminal()
        self.keyboard = Keyboard()
        self.bindings = {
            'q': lambda _: self._do_exit('Quit normally')}
        self.loop = get_event_loop()

    @staticmethod
    def _do_exit(message):
        sys.exit(message)

    def _read_stdin(self):
        key = self.keyboard[self.terminal.infile.read()]
        action = self.bindings.get(key, lambda _: None)
        action(key)

    def run(self):
        if self.terminal.infile.isatty():
            self.loop.add_reader(self.terminal.infile, self._read_stdin)
        with self.terminal.unbuffered_input(), (
                self.terminal.nonblocking_input()):
            try:
                self.loop.run_forever()
            except KeyboardInterrupt:
                self._do_exit('Keyboard interrupt')


def diff(filename_a, filename_b):
    '''Mock app that diffs two audio streams and provides a UI to the user to
    explore that diff.'''
    DiffApp(filename_a, filename_b).run()


if __name__ == '__main__':
    argh.dispatch_command(diff)
