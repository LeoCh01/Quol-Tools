import json

import httpx
from PySide6.QtWidgets import QPushButton, QLineEdit, QHBoxLayout, QLabel, QComboBox, QPlainTextEdit, QVBoxLayout

from lib.quol_window import QuolSubWindow, QuolMainWindow


class MainWindow(QuolMainWindow):

    def __init__(self, window_info, window_context):
        super().__init__('API', window_info, window_context, default_geometry=(930, 10, 200, 1), show_config=False)

        method_layout = QHBoxLayout()
        method_layout.addWidget(QLabel('HTTP Method:'))
        self.method_dropdown = QComboBox(self)
        self.method_dropdown.addItems(['GET', 'POST', 'PUT', 'DELETE', 'PATCH'])
        method_layout.addWidget(self.method_dropdown)
        self.layout.addLayout(method_layout)

        self.url_input = QLineEdit(self)
        self.url_input.setPlaceholderText('API url...')
        self.layout.addWidget(self.url_input)

        self.body_input = QPlainTextEdit(self)
        self.body_input.setPlaceholderText('Body (JSON format)')
        self.body_input.setVisible(False)
        self.layout.addWidget(self.body_input)

        self.headers: dict[str, str] = {}
        self.header_edit = None
        self.headers_button = QPushButton('Headers', self)
        self.headers_button.clicked.connect(self.open_headers_window)
        self.layout.addWidget(self.headers_button)

        self.send_button = QPushButton('Send Request', self)
        self.send_button.clicked.connect(self.send_request)
        self.layout.addWidget(self.send_button)

        self.method_dropdown.currentTextChanged.connect(self.toggle_body_input)

    def toggle_body_input(self):
        method = self.method_dropdown.currentText()
        if method in ['POST', 'PUT', 'PATCH']:
            self.body_input.setVisible(True)
            self.setFixedHeight(260)
        else:
            self.body_input.setVisible(False)
            self.setFixedHeight(162)

    def send_request(self):
        url = self.url_input.text().strip()
        method = self.method_dropdown.currentText()
        body = self.body_input.toPlainText().strip()

        if not url.startswith('http'):
            return

        h = self.headers.copy()
        status_code, text = self.send_request_and_get_response(url, method, body, h)
        self.show_response(status_code, text, url, method, body, h)

    @staticmethod
    def send_request_and_get_response(url, method, body, headers):
        try:
            with httpx.Client() as client:
                if method == 'GET':
                    response = client.get(url, headers=headers)
                elif method == 'POST':
                    response = client.post(url, data=body, headers=headers)
                elif method == 'PUT':
                    response = client.put(url, data=body, headers=headers)
                elif method == 'DELETE':
                    response = client.delete(url, headers=headers)
                elif method == 'PATCH':
                    response = client.patch(url, data=body, headers=headers)

                return response.status_code, response.text
        except httpx.RequestError as e:
            return -1, f"Request failed: {e}"

    def open_headers_window(self):
        header_window = QuolSubWindow(self, "Edit Headers")
        header_window.setGeometry(300, 300, 400, 300)
        header_layout = QVBoxLayout()

        self.header_edit = QPlainTextEdit(header_window)
        self.header_edit.setPlainText(json.dumps(self.headers, indent=2))
        header_layout.addWidget(self.header_edit)

        save_button = QPushButton('Save Headers', header_window)
        save_button.clicked.connect(lambda: self.save_headers(header_window))
        header_layout.addWidget(save_button)

        header_window.layout.addLayout(header_layout)
        header_window.show()

    def save_headers(self, window):
        try:
            updated_headers = json.loads(self.header_edit.toPlainText())
            if not isinstance(updated_headers, dict):
                raise ValueError
            self.headers = {str(k): str(v) for k, v in updated_headers.items()}
            window.close()
        except json.JSONDecodeError:
            return

    def show_response(self, status_code, text, url, method, body, headers):
        response_window = QuolSubWindow(self, str(status_code))
        response_window.setGeometry(200, 200, 600, 400)

        response_text = QPlainTextEdit(response_window)
        response_text.setPlainText(text)
        response_text.setReadOnly(True)

        button_layout = QHBoxLayout()

        resend_button = QPushButton('Resend Request', response_window)
        resend_button.clicked.connect(lambda: self.resend_request(response_window, url, method, body, headers))
        button_layout.addWidget(resend_button)

        edit_button = QPushButton('Edit Request', response_window)
        edit_button.clicked.connect(lambda: self.edit_request(url, method, body, headers))
        button_layout.addWidget(edit_button)

        response_window.layout.addWidget(response_text)
        response_window.layout.addLayout(button_layout)
        response_window.show()

    def resend_request(self, response_window, url, method, body, headers):
        status_code, text = self.send_request_and_get_response(url, method, body, headers)
        response_window.setWindowTitle(str(status_code))
        response_window.layout.itemAt(0).widget().setPlainText(text)

    def edit_request(self, url, method, body, headers):
        self.headers = headers
        self.url_input.setText(url)
        self.method_dropdown.setCurrentText(method)
        self.body_input.setPlainText(body)
