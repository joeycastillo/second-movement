import spotipy
from spotipy.oauth2 import SpotifyClientCredentials
from unidecode import unidecode
import json
from datetime import datetime
import difflib
from fake_useragent import UserAgent
import requests
import time
import pandas as pd
import shutil
import os
sp = None

MAKE_ARR_FILE = 1 # 0 = Don't make the file; 1= Make the file; 2 = Make the file and print it to the console
PRINT_RANKINGs = True
IGNORE_STAGES = False
USE_DAYINFO_FILE = True  # If True, we look for a file called day_info.json and load it. It contains what Clashfinder brings. It gets generated whenever we read from the internet.
USE_LISTACTPOP_FILE = True  # If True, we look for a file called listActsPop.json and load it. It contains what Spotify or Last.fm bring. It gets generated whenever we read from the internet.
PRINT_SEARCH_RESULTS = True
SORT_POP_BY_FOLLOWERS = False
GENRE_DEFAULT = "NO_GENRE"
POPULARITY_SOURCE = 'last.fm'
STAGE_DEFAULT = "NO_STAGE"
URL = 'https://clashfinder.com/data/event/bonnaroo2k26.json'

duoActs = {"Freddy Gibbs & the Alechemist" : ["Freddie Gibbs", "The Alchemist"],
           "LSZEE" : ["LSDREAM", "Clozee"]}
replaceActName = {"Weird Al Yankovich: Bigger & Weirder Ruvue" : "Weird Al Yankovic",
                  "Rachel Chinchouri" : "Rachel Chinouriri",
                  "Waylon Wyett" : "Waylon Wyatt",
                  "Kesha Presents Superjam Esoterica: The Alchemy of Pop" : "Kesha",
                  "Gouldie Boutlier " : "Goldie Boutilier",
                  "A Hundred Dtums" : "A Hundred Drums",
                  "DJ Trixie Mattell" : "Trixie Mattel",
                  "Laszwo" : "Łaszewo",
                  "Hemlock Springs" : "Hemlocke Springs",
                  "Mother, Mother" : "Mother Mother",
                  "Tedeschi-Trucks Band" : "Tedeschi Trucks Band",
                  "Jessie Ware" : "Jessie Murph",
                  "Amyl & the Sniffers" : "Amyl and the Sniffers",
                  "Rufus Du Sol" : "RÜFÜS DU SOL"}
replaceGenres = {   "HIPHOP"           : "RAP",
                    "TRAP"             : "RAP",
                    "CRUNK"            : "RAP",
                    "HARD_ROCK"        : "ROCK",
                    "BLUES_ROCK"       : "ROCK",
                    "INDIE_ROCK"       : "ROCK",
                    "GARAGE_ROCK"      : "ROCK",
                    "HARDCORE"         : "PUNK",
                    "POSTPUNK"         : "PUNK",
                    "RIOT_GRRRL"       : "PUNK",
                    "POP_PUNK"         : "PUNK",
                    "INDIE_POP"        : "POP",
                    "SINGERSONGWRITER" : "POP",
                    "PSYTRANCE"        : "EDM",
                    "MINIMAL_TECHNO"   : "EDM",
                    "FUTURE_BASS"      : "EDM",
                    "MIDTEMPO_BASS"    : "EDM",
                    "ELECTRONIC"       : "EDM",
                    "ELECTROCLASH"     : "EDM",
                    "WITCH_HOUSE"      : "EDM",
                    "DRUM_AND_BASS"    : "DnB",
                    "RNB"              : "SOUL",
                    "BLUES"            : "SOUL",
                    "SOUL"             : "SOUL",
                    "FUNK"             : "SOUL",
                    "BLUEGRASS"        : "FOLK",
                    "CHILLWAVE"        : "ALT",
                    "DREAM_POP"        : "ALT",
                    "SHOEGAZE"         : "ALT",
                    "PSYCHEDELIC"      : "ALT",
                    "INDIE"            : "ALT",
                    "SPANISH"          : "WORLD",
                    "IRISH"            : "WORLD",
                    "DUTCH"            : "WORLD",
                    "UNITED_STATES"    : "WORLD",
                    "8BIT"             : "OTHER",
                    "COMEDY"           : "OTHER"
                }

replaceWords = {
                "RUFUS" : "R;F;S",
                "TROMBONE SHORTY & ORLEANS AVENUE" : "TROMBONE SHORTY",
                "FREDDY GIBBS & THE ALECHEMIST" : "FREDDY GIBBS",
                "BOA" : "B:A",
                "A" : "a",
                "D" : "d",
                "." : "_",
                "&" : "`n",
                "Ü" : ";",
#                "M" : "N%",
#                "W" : "WJ"
                }

def get_artist_popularity_spotify(artist_name, client_id, client_secret):
    # Remove single quotes to avoid search issues
    top_track_not_found = 0
    global sp
    first_match = False
    if "'" in artist_name:
        artist_name = artist_name.replace("'", "")
        first_match = True
    
    if sp is None:
        auth_manager = SpotifyClientCredentials(client_id=client_id, client_secret=client_secret)
        sp = spotipy.Spotify(auth_manager=auth_manager)
    
    result = sp.search(q=f'artist:{artist_name}', type='artist')
    artist_results = result.get('artists', {}).get('items', [])
    
    if not artist_results:
        if PRINT_SEARCH_RESULTS:
            print(f"FAILED TO FIND: {artist_name}")
        return None, top_track_not_found
    
    # Extract artist names for debugging
    names_in_search = [artist['name'] for artist in artist_results]
    # Find the best match
    for i, artist in enumerate(artist_results):
        if first_match:
            if PRINT_SEARCH_RESULTS:
                top_tracks = sp.artist_top_tracks(artist['id'], country='US')
                top_track = top_tracks['tracks'][0]['name'] if top_tracks['tracks'] else top_track_not_found
                print(f"Best for {artist_name} is {artist['name']} | Chose element {i} in {names_in_search} | Top Track is: {top_track}")
            return artist, top_track
        
        if artist['name'].upper() == artist_name.upper():
            if PRINT_SEARCH_RESULTS:
                top_tracks = sp.artist_top_tracks(artist['id'], country='US')
                top_track = top_tracks['tracks'][0]['name'] if top_tracks['tracks'] else top_track_not_found
                print(f"Found {artist['name']} | Chose element {i} in {names_in_search} | Top Track is: {top_track}")
            return artist, top_track
    
    # No exact match found
    if PRINT_SEARCH_RESULTS:
        print(f"FAILED TO FIND: {artist_name}, BUT FOUND {names_in_search}")
    
    return None, top_track_not_found

def get_artist_followers_popularity_lastfm(artist_name, client_id, client_secret):
    while True:
        base_url = "http://ws.audioscrobbler.com/2.0/"
        api_key = client_secret
        params = {
            "method": "artist.getinfo",
            "artist": artist_name,
            "api_key": api_key,
            "format": "json",
            "autocorrect": 1 
        }
        r = requests.get(base_url, params=params)
        try:
            data = r.json()
        except requests.exceptions.JSONDecodeError:
            print(f"Error for {artist_name}: {r.content.decode()}")
            if ("Please try again in 30 seconds" in r.content.decode()):
                print("Waiting 30 seconds and retrying")
                time.sleep(30)
                continue
            data = []
        if "artist" not in data:
            return 0, 0, 0, [], artist_name, False
        artist = data["artist"]
        # Followers → listeners
        try:
            followers = int(artist["stats"]["listeners"])
        except:
            followers = 0
        # Popularity proxy → playcount (or adjust below)
        try:
            playcount = int(artist["stats"]["playcount"])
        except:
            playcount = 0

        popularity = playcount
        # alternative:
        # popularity = playcount / followers if followers else 0

        # Genres → tags
        name = artist.get("name", artist_name)
        tags = artist.get("tags", {}).get("tag", [])
        genre_list = [t["name"] for t in tags]

        # --- Top track ---
        top_song = get_top_track_lastfm(name, api_key, base_url)
        print(f"Found {name} | Top Track is: {top_song}")
        return followers, popularity, top_song, genre_list, name, True


def get_top_track_lastfm(artist_name, api_key, base_url):
    while True:
        params = {
            "method": "artist.gettoptracks",
            "artist": artist_name,
            "api_key": api_key,
            "format": "json",
            "limit": 1
        }
        r = requests.get(base_url, params=params)
        try:
            data = r.json()
        except requests.exceptions.JSONDecodeError:
            print(f"Error for Top Song of {artist_name}: {r.content.decode()}")
            if ("Please try again in 30 seconds" in r.content.decode()):
                print("Waiting 30 seconds and retrying")
                time.sleep(30)
                continue
            return 0
        try:
            return data["toptracks"]["track"][0]["name"]
        except (KeyError, IndexError):
            return 0

# Simpler functions

def get_spotify_credentials(file_path="creds.json"):
    try:
        with open(file_path, 'r') as file:
            data = json.load(file)
            client_id = data['spotify']['client_id']
            client_secret = data['spotify']['client_secret']
            return client_id, client_secret
    except (KeyError, FileNotFoundError, json.decoder.JSONDecodeError) as e:
        print(f"No Spotify credentials found. {e}. Setting all acts to a popularity of 0.")
        return None, None

def get_lastfm_credentials(file_path="creds.json"):
    try:
        with open(file_path, 'r') as file:
            data = json.load(file)
            client_id = data['last.fm']['client_id']
            client_secret = data['last.fm']['client_secret']
            return client_id, client_secret
    except (KeyError, FileNotFoundError, json.decoder.JSONDecodeError) as e:
        print(f"No last.fm credentials found. {e}. Setting all acts to a popularity of 0.")
        return None, None

def get_clashfinder_credentials(file_path="creds.json"):
    try:
        with open(file_path, 'r') as file:
            data = json.load(file)
            username = data['clashfinder']['username']
            public_key = data['clashfinder']['public_key']
            return username, public_key
    except (KeyError, FileNotFoundError, json.decoder.JSONDecodeError) as e:
        print(f"No clashfinder credentials found. {e}.")
        return None, None

def rank_artists_by_popularity(artists):
    # Sort artists by popularity in descending order
    sorted_artists = sorted(artists, key=lambda x: x['followers'], reverse=True)
    artist_ranks = {}
    # Assign ranks starting from 1
    for rank, artist in enumerate(sorted_artists, start=1):
        artist_ranks[artist['name']] = rank
    return artist_ranks

def get_ranking(artists, name):
    for artist in artists:
        if artist['name'].lower() == name.lower():
            return artist['overall']
    return 0

def set_pop_follow_manually(artists, name, pop, follower):
    for artist in artists:
        if artist['name'].lower() == name.lower():
            artist['popularity'] = pop
            artist['followers'] = follower
    return artists

def get_url_content(url):
    try:
        ua = UserAgent()
        user_agent = ua.chrome
        response = requests.get(url, headers={'User-Agent': user_agent}, verify=False)
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        print(f"Error fetching data: {e}")
        return None

def get_artist_followers_popularity(artist_name, client_id, client_secret):
    if POPULARITY_SOURCE == "spotify":
        artist_info, top_song = get_artist_popularity_spotify(artist_name, client_id, client_secret)
        if artist_info is None:
            return 0,0,0,[], artist_name, False
        name_found = artist_info['name']
        followers = artist_info['followers']['total']
        popularity = artist_info['popularity']
        genre_list = artist_info['genres']
        return followers, popularity, top_song, genre_list, name_found, True
    elif POPULARITY_SOURCE == "last.fm":
        time.sleep(0.25)
        return get_artist_followers_popularity_lastfm(artist_name, client_id, client_secret)

def strip_word(text, words_to_remove, startsWithOnly=False):
    if startsWithOnly:
        for word in words_to_remove:
            if text.lower().startswith(f"{word.lower()} "):
                return text[len(word)+1:]
        return text
    words = text.split()
    stripped_words = [word for word in words if word.lower() not in map(str.lower, words_to_remove)]
    stripped_text = ' '.join(stripped_words)
    return stripped_text

def writeAndPrint(file, text):
    file.write(f"{text}\n")
    if MAKE_ARR_FILE == 2:
        print(text)
        
def get_value_for_artist(wanted_act, key, array):
    for artist in array:
        if artist['name'] == wanted_act:
            return artist[key]
    return 0

# More complex functions

def get_html_data(content):
    fullData = {}
    date_str = "%Y-%m-%d %H:%M"
    locations = content.get('locations')
    for venue in locations:
        stage_name = venue.get('name')
        stage_name = stage_name.upper()
        stage_name = stage_name.replace(" ", "_")
        events = venue.get('events')
        for act in events:
            artist = act.get('name')
            artist = replaceActName.get(artist, artist)
            start_time = datetime.strptime(act.get('start'), date_str)
            end_time = datetime.strptime(act.get('end'), date_str)
            act_info = {"stage" : stage_name, "start_time" : start_time, "end_time" : end_time}
            if artist in fullData:  # For when an act has multiple sets
                if isinstance(fullData[artist], dict):
                    fullData[artist] = [fullData[artist]]
                fullData[artist].append(act_info)
            else:
                fullData[artist] = act_info
    return fullData


def dates_to_act(act, day_info, element = 0):
    defaultStart = datetime(2024, 6, 20, 15, 0)
    defaultEnd = datetime(2024, 6, 20, 16, 0)
    datInfoAct = day_info[act]
    timesTheyPlay = 1
    if isinstance(datInfoAct, list):
        timesTheyPlay = len(datInfoAct)
        datInfoAct = datInfoAct[element]
    stage = datInfoAct.get("stage")
    return stage, datInfoAct["start_time"], datInfoAct["end_time"], timesTheyPlay

def popularity_sort(popList):
    if not popList:
        return popList
    use_monthly_listens = "monthly_listeners" in popList[0]
    if use_monthly_listens:
        pop_weights = [0.4, 0.3, 0.3]  # populary, follower, monthly listens 
    elif POPULARITY_SOURCE == "spotify":
        pop_weights = [0.7, 0.3, 0]  # populary, follower, N/A
    elif POPULARITY_SOURCE == "last.fm":
        pop_weights = [0.8, 0.2, 0]  # populary, follower, N/A
    maxPop = max(max(artist['popularity'] for artist in popList), 1)
    maxFol = max(max(artist['followers'] for artist in popList), 1)
    if use_monthly_listens:
        maxMonLis = max(max(artist['monthly_listeners'] for artist in popList), 1)
    else:
        fol_pop_ratio = ((pop_weights[0] / pop_weights[1]) * (maxFol / maxPop))
        if fol_pop_ratio >= 1:
            print(f"Ratio for prioritizing followers to popularity: {fol_pop_ratio:.2f} followers for 1 pop")
        else:
            fol_pop_ratio = 1 / fol_pop_ratio
            print(f"Ratio for prioritizing popularity to followers: {fol_pop_ratio:.2f} followers for 1 pop")
    for pop in popList:
        popPercent = pop['popularity']/maxPop
        folPercent = pop['followers']/maxFol
        monLisPercent = pop['monthly_listeners']/maxMonLis if use_monthly_listens else 0
        values = [popPercent, folPercent, monLisPercent]
        pop["fullPop"] = sum(v * w for v, w in zip(values, pop_weights))
    popList = sorted(popList, key=lambda x: x["fullPop"], reverse=True)
    for i, artist in enumerate(popList):
        artist['overall'] = i + 1 
    return popList


def getFullArray(listActs):
    # Spits out the array of acts in alphabetically order
    duo_mult = 1.2  # Since duos are more hype, add a multiplier to their popularity and follower averages
    listActsPop = []
    listActsNotFound = []
    listActsInListDuos = []
    if POPULARITY_SOURCE == "spotify":
        client_id, client_secret = get_spotify_credentials()
    elif POPULARITY_SOURCE == "last.fm":
        client_id, client_secret = get_lastfm_credentials()
    if client_id is None and client_secret is None:  # If there aren't any Spotify credentials, just set the popularity of all acts to zero
        for act in listActs:
            listActsPop.append({'name':act, 'followers' : 0, "popularity" : 0, "top_song" : None, "genre_list" : []})
        return listActsPop
    for act in listActs:
        act_spot = duoActs.get(act, act)
        if isinstance(act_spot,list):
            listActsInListDuos.append(act)
            continue # We'll get and average the duos later.
        followers, popularity, top_song, genre_list, name_found, found = get_artist_followers_popularity(act_spot, client_id, client_secret)
        listActsPop.append({'name' : act, 'name_found' : name_found, 'followers' : followers, "popularity" : popularity, "top_song" : top_song, "genre_list" : genre_list})
        if not found:
            listActsNotFound.append(act_spot)
    # This logic is to average duos
    for duo in duoActs:
        if duo not in listActsInListDuos:
            continue
        artists = duoActs[duo]
        if not isinstance(artists,list):
            continue
        followers = 0
        popularity = 0
        genre_list = []
        for artist in artists:
            fol, pop, top_song, genres, name_found, found = get_artist_followers_popularity(artist, client_id, client_secret)
            if not found:
                listActsNotFound.append(act_spot)
            else:
                listActsPop.append({'name' : artist, 'name_found' : name_found, 'followers' : fol, "popularity" : pop, "top_song" : top_song, "genre_list" : genres})
                genre_list.extend(genres)
                followers += fol
                popularity += pop
        followers = int((followers / len(artists)) * duo_mult)
        popularity = int((popularity / len(artists)) * duo_mult)
        listActsPop.append({'name':duo, 'name_found' : duo, 'followers' : followers, "popularity" : popularity, "top_song" : artists, "genre_list" : genre_list})
        if not found:
            listActsNotFound.append(act_spot)
    if PRINT_SEARCH_RESULTS and len(listActsNotFound) > 0:
        print(f"Unable to find the following Acts: {listActsNotFound}")
    return listActsPop

def print_md_lst(sorted_listing, day_info):
    displayStage = False #True if day_info else False
    displayTopSong = 'top_song' in sorted_listing[0]
    displayGenre = 'genre_list' in sorted_listing[0]
    displayMonLis = 'monthly_listeners' in sorted_listing[0]
    numTitle = "Num"
    actTitle = "Act"
    popTitle = "Popularity"
    folTitle = "Followers"
    monLisTitle = "Monthly Listeners"
    genreTitle = "Genre"
    topSongTitle = "Popular Song"
    stgTitle = "Stage"
    longestNum = len(numTitle) + 2
    longestAct = len(actTitle) + 2
    longestPop = len(popTitle) + 2
    longestFol = len(folTitle) + 2
    longestSng = len(topSongTitle) + 2
    longestGen = len(genreTitle) + 2
    longestStg = len(stgTitle) + 2
    longestMonLis = len(monLisTitle) + 2
    stageDict = {}
    artist_genre = {} # Independent dict to avoid side-effects
    for item in sorted_listing:
        if displayStage:
            stage, _, _, _ = dates_to_act(item['name_found'], day_info)
            if stage is False:
                continue
            stageDict[item['name']] = stage
        longestAct = max(longestAct, len(item['name']))
        longestPop = max(longestPop, len(str(item['popularity'])))
        longestFol = max(longestFol, len(str(item['followers'])))
        if displayMonLis:
            longestMonLis = max(longestMonLis, len(str(item['monthly_listeners'])))
        if displayGenre:
            artist_genre[item['name']] = "-" if not item['genre_disp'] else item['genre_disp']
            longestGen = max(longestGen, len(str(artist_genre[item['name']])))
        if displayTopSong:
            top_song = str(item['top_song'])
            if ")" in top_song: # If you can't make your song title work after one set of paranthesis, you don't deserve more text.
                item['top_song'] = top_song.split(')')[0] + ')'
            longestSng = max(longestSng, len(str(item['top_song'])))
        if displayStage:
            longestStg = max(longestStg, len(stage))
    printText = f"| {numTitle: ^{longestNum}} | {actTitle: ^{longestAct}} | {popTitle : ^{longestPop}} | {folTitle : ^{longestFol}} |"
    if displayMonLis:
        printText += f" {monLisTitle : ^{longestMonLis}} |"
    if displayGenre:
        printText += f" {genreTitle : ^{longestGen}} |"
    if displayTopSong:
        printText += f" {topSongTitle : ^{longestSng}} |"
    if displayStage:
        printText += f" {stgTitle : ^{longestStg}} |"
    print(printText)
    printText = f"| {'-' * longestNum} | {'-' * longestAct} | {'-' * longestPop} | {'-' * longestFol} |"
    if displayMonLis:
        printText += f" {'-' * longestMonLis} |"
    if displayGenre:
        printText += f" {'-' * longestGen} |"
    if displayTopSong:
        printText += f" {'-' * longestSng} |"
    if displayStage:
        printText += f" {'-' * longestStg} |"
    print(printText)
    for item in sorted_listing:
        act = item['name_found']
        popularity = item['popularity']
        followers = item['followers']
        overall = item['overall']
        printText = f"| {overall : ^{longestNum}} | {act : ^{longestAct}} | {popularity : ^{longestPop}} | {followers : ^{longestFol}} |"
        if displayMonLis:
            monthly_listeners = item['monthly_listeners']
            printText += f" {monthly_listeners : ^{longestMonLis}} |"
        if displayGenre:
            genre = artist_genre[item['name']]
            printText += f" {genre : ^{longestGen}} |"
        if displayTopSong:
            top_song = item['top_song']
            printText += f" {top_song : ^{longestSng}} |"
        if displayStage:
            stage = stageDict[item['name']]
            printText += f" {stage : ^{longestStg}} |"
        print(printText)

def get_name_to_display(act, i=0, maxDisplayLen=None):
    actToDisp = unidecode(act)
    actToDisp = strip_word(actToDisp,["The"], True)
    actToDisp = actToDisp.upper()
    for was, want in replaceWords.items():
        actToDisp = actToDisp.replace(was, want)
    if i > 0:
        actToDisp = f"{actToDisp} {i+1}"
    if maxDisplayLen is not None:
        actToDisp = f"{actToDisp[:maxDisplayLen]}"
    actToDisp = f"{actToDisp :<6}"  # <6 makes it so text that won't fill the whole watch screen will be filled with spaces
    return actToDisp

def print_array_for_watch(listActs, sorted_listing, day_info, filename):
    list_stages = sorted({
        info_iter["stage"]
        for info in day_info.values()
        for info_iter in (info if isinstance(info, list) else [info])
        if info_iter.get("stage") is not None
    })
    dict_genres = {}
    list_genres = []
    for info in sorted_listing:
        if info["genre_disp"] is None:
            dict_genres[info["name"]] = "NONE"
        else:
            genre = info["genre_disp"].upper()
            genre = genre.replace(" ", "_")
            genre = genre.replace("-", "")
            genre = replaceGenres.get(genre, genre)
            dict_genres[info["name"]] = genre
            if genre not in list_genres:
                list_genres.append(genre)
    list_genres = sorted(list_genres)
    enum_txt_start = 'FESTIVAL_SCHEDULE_'
    if not day_info:
        print("Cannot print date array for watch; no date info found")
        return
    maxDisplayLen = None  # If None, then use the max length of the options, but 21 is a good option too.
    with open(f'{filename}.txt', 'w') as f:

        writeAndPrint(f, "##############PASTE THIS PART INTO festival_schedule_face.h###############")
        writeAndPrint(f, "typedef enum {")
        writeAndPrint(f, f"    {enum_txt_start}NO_STAGE = 0,")
        for stage in list_stages:
            writeAndPrint(f, f"    {enum_txt_start}{stage},")
        writeAndPrint(f, f"    {enum_txt_start}STAGE_COUNT")
        writeAndPrint(f, "} festival_schedule_stage;")
        writeAndPrint(f, "")
        writeAndPrint(f, "typedef enum {")
        writeAndPrint(f, f"    {enum_txt_start}NO_GENRE = 0,")
        for genre in list_genres:
            writeAndPrint(f, f"    {enum_txt_start}{genre},")
        writeAndPrint(f, f"    {enum_txt_start}GENRE_COUNT")
        writeAndPrint(f, "} festival_schedule_genre;")
        writeAndPrint(f, "###########################################################################")



        artistDateNotFound = []
        writeAndPrint(f, f"// Line-up - {URL}")
        writeAndPrint(f, '#include "festival_schedule_face.h"')
        writeAndPrint(f, "")
        totalActs = 0
        listActsPrint = []
        for act in listActs:
            stage, start, end, timesPerformed = dates_to_act(act, day_info, element=0)
            order_single_act_performances = []
            for i in range(timesPerformed):
                stage, start, end, _ = dates_to_act(act, day_info, element=i)
                if stage == STAGE_DEFAULT:
                    artistDateNotFound.append(act)
                    continue
                order_single_act_performances.append({'start':start, 'end':end, 'stage':stage})
            order_single_act_performances.sort(key=lambda x: list_stages.index(x['stage']))
            for i, performance in enumerate(order_single_act_performances):
                actToDisp = get_name_to_display(act, i, maxDisplayLen)
                listActsPrint.append({'dispAct': actToDisp, 'act': act, 'start': performance['start'], 'end': performance['end'], 'stage': performance['stage']})
                totalActs += 1
        if maxDisplayLen is None:
            arrLenSchedule = len(max(listActsPrint, key=lambda x: len(x['dispAct']))['dispAct']) + 2  # +2 for padding/EOL
            arrActLongestSchedule = max(listActsPrint, key=lambda x: len(x['dispAct']))['dispAct']
            print(f"Set festival_schedule_t.artist's length to {arrLenSchedule} due to {arrActLongestSchedule}")
        listActsPrint = sorted(listActsPrint, key=lambda d: d['dispAct'].lstrip().lower())
        writeAndPrint(f, f"#define {enum_txt_start}NUM_ACTS {totalActs}")
        writeAndPrint(f, "")
        writeAndPrint(f, f"const festival_schedule_t festival_acts[{enum_txt_start}NUM_ACTS + 1]=")
        writeAndPrint(f, "{")
        for actData in listActsPrint:     
            writeAndPrint(f, "    {")
            writeAndPrint(f, f'        .artist = "{actData["dispAct"]}",')
            writeAndPrint(f, f'        .stage = {enum_txt_start}{actData["stage"].upper()},')
            writeAndPrint(f, f'        .start_time = {{.unit.year = {actData["start"].year - 2020}, .unit.month = {actData["start"].month}, .unit.day = {actData["start"].day}, .unit.hour = {actData["start"].hour}, .unit.minute = {actData["start"].minute}}},')
            writeAndPrint(f, f'        .end_time = {{.unit.year = {actData["end"].year - 2020}, .unit.month = {actData["end"].month}, .unit.day = {actData["end"].day}, .unit.hour = {actData["end"].hour}, .unit.minute = {actData["end"].minute}}},')
            writeAndPrint(f, f'        .genre = {enum_txt_start}{dict_genres[actData["act"]]},')
            writeAndPrint(f, f'        .popularity = {get_ranking(sorted_listing, actData["act"])}')
            writeAndPrint(f, "    },")
        writeAndPrint(f, f'    [{enum_txt_start}NUM_ACTS]  = {{ //Fall back')
        writeAndPrint(f, '        .artist = "No Act",')
        writeAndPrint(f,f'        .stage = {enum_txt_start}STAGE_COUNT,')
        writeAndPrint(f, '        .start_time = {.unit.year = 0, .unit.month = 0, .unit.day = 0, .unit.hour = 0, .unit.minute = 0},')
        writeAndPrint(f, '        .end_time = {.unit.year = 63, .unit.month = 15, .unit.day = 31, .unit.hour = 31, .unit.minute = 63},')
        writeAndPrint(f,f'        .genre = {enum_txt_start}GENRE_COUNT,')
        writeAndPrint(f, '        .popularity = 0')
        writeAndPrint(f, '    }')
        writeAndPrint(f, '};')
    
    if PRINT_SEARCH_RESULTS and artistDateNotFound:
        print(f"\nFAILED TO FIND DATE INFO FOR {artistDateNotFound}\n")

def show_correlation(sorted_listing):
    df = pd.DataFrame(sorted_listing)
    items = ['monthly_listeners', 'popularity', 'followers'] if "monthly_listeners" in sorted_listing[0] else ['popularity', 'followers']
    correlation = df[items].corr()
    print(correlation)


if __name__ == "__main__":
    if IGNORE_STAGES:
        day_info = {}
    else:
        if USE_DAYINFO_FILE:
            with open("day_info.json", "r") as f:
                html_str = json.load(f)
        else:
            username, public_key = get_clashfinder_credentials()
            clashfinder_url = f"{URL}?authUsername={username}&authPublicKey={public_key}"
            html_str = get_url_content(clashfinder_url)
            with open("day_info.json", "w") as f:
                json.dump(html_str, f, indent=2)
        day_info = get_html_data(html_str)
        day_info = {key.split(" (")[0]: value for key, value in day_info.items()}

    listActs = sorted(list(day_info.keys()))
    listActsWatch = sorted(listActs, key=lambda x: x.lower().replace("the ", "") if x[0].startswith("the ") else x[0].lower())
    if USE_LISTACTPOP_FILE:
        with open("listActsPop.json", "r") as f:
            listActsPop = json.load(f)
    else:
        start_time = time.perf_counter()
        listActsPop = getFullArray(listActs)
        print(f"Getting All Artist Pop Data took: {((time.perf_counter() - start_time) / 60):.2f} minutes")
        for item in listActsPop:
            item['genre_disp'] = item['genre_list'][0] if item['genre_list'] else None
        if os.path.exists("listActsPop.json"):
            shutil.copy2("listActsPop.json", "listActsPop_bak.json")
            print(f"Saved copy of previous listActsPop file")
        with open("listActsPop.json", "w") as f:
            json.dump(listActsPop, f, indent=2)
        #  We store the individual acts that make up the duo in the JSON for review, so remove them.
    for i in reversed(range(len(listActsPop))):
        act_name = listActsPop[i]['name']
        if any(act_name in lst for lst in duoActs.values()):
            listActsPop.pop(i)
    if SORT_POP_BY_FOLLOWERS:  
        sortKey = lambda x: (x['followers'])
        sorted_listing = sorted(listActsPop, key=sortKey, reverse=True)
    else:
        sortKey = lambda x: (x['popularity'], x['followers']) # Sort by Spotify popularity w/ followers being the tie-breaker
        sorted_listing = sorted(listActsPop, key=sortKey, reverse=True)
        sorted_listing = popularity_sort(sorted_listing)
    if PRINT_RANKINGs: 
        print_md_lst(sorted_listing, day_info)
    if MAKE_ARR_FILE:
        print_array_for_watch(listActsWatch, sorted_listing, day_info ,"festival_schedule_arr")

    show_correlation(sorted_listing)
