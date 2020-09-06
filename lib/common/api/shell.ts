import { deprecate } from 'electron/main';

const shell = process._linkedBinding('electron_common_shell');

shell.moveItemToTrash = deprecate.renameFunction(shell.moveItemToTrash, 'shell.trashItem');

export default shell;
