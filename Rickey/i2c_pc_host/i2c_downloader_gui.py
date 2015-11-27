# File: fileseldlg.py
#    http://infohost.nmt.edu/tcc/help/pubs/tkinter//dialogs.html#tkFileDialog
#    http://tkinter.unpythonic.net/wiki/tkFileDialog
#
# Note:
#    there are a variety of options for the FileDialog
#    see above references for more information

from tkinter import *
from tkinter import ttk
import tkinter.filedialog as fdlg

from demopanels import MsgPanel, SeeDismissPanel

class FileSelDlgDemo(ttk.Frame):

    def __init__(self, isapp=True, name='fileseldlgdemo'):
        ttk.Frame.__init__(self, name=name)
        self.pack(expand=Y, fill=BOTH)
        self.master.title('File Selection Dialog Demo')
        self.isapp = isapp
        self._create_widgets()

    def _create_widgets(self):
        if self.isapp:
            MsgPanel(self,
                     ["Enter a file name in the entry box or click on the 'Browse' ",
                      "buttons to select a file name using the file selection dialog."])

            SeeDismissPanel(self)

        self._create_demo_panel()

    def _create_demo_panel(self):
        demoPanel = Frame(self)
        demoPanel.pack(side=TOP, fill=BOTH, expand=Y)

        for item in ('open', 'save'):
            frame = ttk.Frame(demoPanel)
            lbl = ttk.Label(frame, width=20,
                            text='Select a file to {} '.format(item))
            ent = ttk.Entry(frame, width=25)
            btn = ttk.Button(frame, text='Browse...',
                             command=lambda i=item, e=ent: self._file_dialog(i, e))
            lbl.pack(side=LEFT)
            ent.pack(side=LEFT, expand=Y, fill=X)
            btn.pack(side=LEFT, padx=5)
            frame.pack(fill=X, padx='1c', pady=3)

    def _file_dialog(self, type, ent):
        # triggered when the user clicks a 'Browse' button
        fn = None
        opts = {'initialfile': ent.get(),
                'filetypes': (('Python files', '.py'),
                              ('PNG', '.png'),
                              ('Text files', '.txt'),
                              ('All files', '.*'),)}

        if type == 'open':
            opts['title'] = 'Select a file to open...'
            fn = fdlg.askopenfilename(**opts)
        else:
            # this should only return a filename; however,
            # under windows, selecting a file and hitting
            # 'Save' gives a warning about replacing an
            # existing file; although selecting 'Yes' does
            # not actually cause a 'Save'; the filename
            # is simply returned
            opts['title'] = 'Select a file to save...'
            fn = fdlg.asksaveasfilename(**opts)

        if fn:
            ent.delete(0, END)
            ent.insert(END, fn)


if __name__ == '__main__':
    FileSelDlgDemo().mainloop()