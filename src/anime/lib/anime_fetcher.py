import httpx
from bs4 import BeautifulSoup


def get_updated_anime(page=1, sub_only=False):
    res = httpx.get(f'https://9animetv.to/recently-updated?page={page}')
    soup = BeautifulSoup(res.text, 'html.parser')
    items = soup.find_all('div', class_='flw-item')

    data = []

    for x in items:
        title_tag = x.find('h3', class_='film-name')
        title = title_tag.get_text(strip=True) if title_tag else ''

        episode_tag = x.find('div', class_='tick-eps')
        episode = episode_tag.get_text(strip=True) if episode_tag else ''

        sub_tag = x.find('div', class_='tick-sub')
        dub_tag = x.find('div', class_='tick-dub')
        sub = 'SUB' if sub_tag else ''
        dub = 'DUB' if dub_tag else ''

        if not sub and sub_only:
            continue

        img_tag = x.find('img', class_='film-poster-img')
        img_url = img_tag['data-src'] if img_tag else 'No Image'

        anime_url_tag = x.find('a', class_='film-poster-ahref')
        anime_url = "https://9animetv.to" + anime_url_tag['href'] if anime_url_tag else ''

        data.append({
            'title': title,
            'episode': episode,
            'sub': sub,
            'dub': dub,
            'img_url': img_url,
            'anime_url': anime_url
        })

    return data
