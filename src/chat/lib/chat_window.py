from markdown import markdown
from pygments.formatters.html import HtmlFormatter
from PySide6.QtCore import QRect
from PySide6.QtGui import QGuiApplication
from PySide6.QtWidgets import QApplication, QTextBrowser


from lib.quol_window import QuolSubWindow


class ChatWindow(QuolSubWindow):
    def __init__(self, main_window):
        super().__init__(main_window, 'Chat')
        screen_geometry = QGuiApplication.primaryScreen().geometry()
        self.setGeometry(QRect(screen_geometry.width() - 510, screen_geometry.height() - 610, 500, 600))

        with open(main_window.window_info.path + '/res/styles.css') as f:
            self.style_tag = f'<style>{HtmlFormatter(style='monokai').get_style_defs('.codehilite')}{f.read()}</style>'

        self.chat_response = QTextBrowser()
        self.chat_response.setOpenExternalLinks(True)
        self.chat_response.setReadOnly(True)
        self.layout.addWidget(self.chat_response, stretch=1)

    def set_output(self, text=''):
        data = ''.join(
            f'''
                <table width="100%">
                  <tr>
                    <td align="{'left' if h['role'] == 'model' else 'right'}" class="{'ai-block' if h['role'] == 'model' else 'user-block'}"><div>{markdown(h['text'], extensions=["fenced_code", "codehilite"])}</div></td>
                  </tr>
                </table>
            ''' for h in self.main_window.ai.history + [{'role': 'model', 'text': text}]
        )

        scrollbar = self.chat_response.verticalScrollBar()
        scroll_pos = scrollbar.value()

        formatted_text = f'{self.style_tag}<body>{data}</body>'
        self.chat_response.setHtml(formatted_text)

        scrollbar.setValue(scroll_pos)
        self.chat_response.repaint()
        QApplication.processEvents()

    def scroll_to_bottom(self):
        scrollbar = self.chat_response.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())

    def closeEvent(self, event):
        print('Window closed')
        super().closeEvent(event)
        # self.setGeometry(QRect(self.g[0], self.g[1], self.g[2], self.g[3]))
        self.main_window.ai.history.clear()
        self.close()
