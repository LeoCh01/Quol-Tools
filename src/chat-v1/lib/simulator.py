from selenium.webdriver.common.by import By
from selenium.webdriver.support import expected_conditions as EC
from selenium.common.exceptions import StaleElementReferenceException, NoSuchElementException
from selenium.webdriver.support.wait import WebDriverWait
from seleniumbase import Driver

MAX_RETRIES = 3


class Simulator:
    def __init__(self):
        self.driver = None
        self.responses = []
        self.use = 'chatgpt'
        self.is_loaded = False

    def reload(self, use, headless=True):
        if self.driver:
            try:
                self.driver.quit()
            except Exception as e:
                print('Error quitting driver:', e)

        self.driver = Driver(uc=True, headless=headless, disable_csp=True)
        self.use = use
        self.is_loaded = False
        self.responses = []

        if self.use == 'chatgpt':
            self.driver.uc_open('https://chat.openai.com/')

            if self.driver.is_element_visible('[data-testid="close-button"]'):
                self.driver.find_element(By.CSS_SELECTOR, '[data-testid="close-button"]').click()

        elif self.use == 'grok':
            self.driver.uc_open('https://grok.com')

        self.is_loaded = True
        print('AI loaded')

    def refresh(self):
        try:
            if self.driver:
                self.driver.refresh()
                self.responses = []
        except Exception as e:
            print('Error refreshing, reloading:', e)
            self.reload(self.use)

    def close(self):
        self.is_loaded = False
        if self.driver:
            try:
                self.driver.quit()
            except Exception as e:
                print('Error quitting driver:', e)
            self.driver = None

    def submit(self, msg, img_path=None):
        if not self.driver:
            return 'Error: AI not loaded.'

        if self.use == 'chatgpt':
            return self.gpt(msg, img_path)
        elif self.use == 'grok':
            return self.grok(msg, img_path)

    def gpt(self, msg, img_path=None) -> str:
        text_input = self.driver.find_element(By.CLASS_NAME, 'ProseMirror')
        text_input.send_keys(msg)

        if img_path:
            file_input = self.driver.find_element(By.CSS_SELECTOR, 'input[type="file"]')
            file_input.send_keys(img_path)

        wait = WebDriverWait(self.driver, 30)

        for attempt in range(MAX_RETRIES):
            try:
                # print(f"Looking for submit button (attempt {attempt + 1})...")
                button = wait.until(EC.element_to_be_clickable((By.ID, "composer-submit-button")))
                button.click()
                break
            except StaleElementReferenceException:
                # print(f"StaleElementReferenceException on attempt {attempt + 1}, retrying...")
                continue
        else:
            # print("Failed to click the submit button after retries.")
            return 'Error: Unable to submit the prompt.'

        # print('Waiting for ".markdown" tag...')
        wait.until(EC.presence_of_element_located((By.CLASS_NAME, 'markdown')))

        while True:
            try:
                latest = self.driver.find_elements(By.CLASS_NAME, 'markdown')[-1]
                yield latest.get_attribute('outerHTML')
            except (StaleElementReferenceException, NoSuchElementException):
                # print('Element error (stale or missing), retrying...')
                continue
            except Exception as e:
                # print('Unexpected error:', e)
                continue

            try:
                # print('Waiting for response stream to complete...')
                self.driver.find_element(By.CSS_SELECTOR, '[aria-label="Stop streaming"]')
            except NoSuchElementException:
                # print('No stop button found, assuming response complete.')
                break

        # print('Response received, processing...')

        try:
            last_response = self.driver.find_elements(By.CLASS_NAME, 'markdown')[-1].get_attribute('outerHTML')
            self.responses.append(last_response)
            yield last_response
        except Exception as e:
            # print("Failed to capture final response:", e)
            return 'Error: Unable to capture final response.'

        # print('Final response processing complete.')

    def grok(self, msg, img_path=None):
        return ''
