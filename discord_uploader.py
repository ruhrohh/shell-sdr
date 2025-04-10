import os
import sys
import json
import discord
import glob
from discord.ext import commands
from dotenv import load_dotenv

# Load Discord token from environment file
load_dotenv()
TOKEN = os.getenv('DISCORD_TOKEN')

# Channel IDs for different file types
SPECTRUM_CHANNEL_ID = int(os.getenv('SPECTRUM_CHANNEL_ID'))
IQ_CHANNEL_ID = int(os.getenv('IQ_CHANNEL_ID'))
SNR_CHANNEL_ID = int(os.getenv('SNR_CHANNEL_ID'))

# Tracking file path
TRACKING_FILE = 'uploaded_files.json'

# Set up the bot
intents = discord.Intents.default()
intents.message_content = True
bot = commands.Bot(command_prefix='!', intents=intents)

def load_uploaded_files():
    """Load the list of already uploaded files"""
    if os.path.exists(TRACKING_FILE):
        with open(TRACKING_FILE, 'r') as f:
            return json.load(f)
    return []

def save_uploaded_files(uploaded_files):
    """Save the updated list of uploaded files"""
    with open(TRACKING_FILE, 'w') as f:
        json.dump(uploaded_files, f)

async def upload_files(channel, file_paths, file_type):
    """Helper function to upload files to a specific channel"""
    # Load already uploaded files
    uploaded_files = load_uploaded_files()

    # Filter out already uploaded files
    new_files = [f for f in file_paths if f not in uploaded_files]

    if not new_files:
        print(f"No new {file_type} files to upload.")
        return 0

    print(f"Uploading {len(new_files)} new {file_type} files...")

    for file_path in new_files:
        filename = os.path.basename(file_path)
        try:
            await channel.send(f"New {file_type} file: {filename}")
            await channel.send(file=discord.File(file_path))
            uploaded_files.append(file_path)  # Add to uploaded list
        except Exception as e:
            print(f"Error uploading {filename}: {e}")

    # Save updated list
    save_uploaded_files(uploaded_files)

    print(f"Uploaded {len(new_files)} new {file_type} files successfully.")
    return len(new_files)

@bot.event
async def on_ready():
    print(f'{bot.user} has connected to Discord!')

    # Get file type from command line
    file_type = 'all'
    if len(sys.argv) > 1:
        file_type = sys.argv[1]

    data_dir = './data'
    total_uploaded = 0

    # Upload files based on type
    if file_type in ['spectrum', 'all']:
        spectrum_files = glob.glob(f"{data_dir}/spectrum_logs/*.csv")
        spectrum_channel = bot.get_channel(SPECTRUM_CHANNEL_ID)
        count = await upload_files(spectrum_channel, spectrum_files, "spectrum")
        total_uploaded += count

    if file_type in ['iq', 'all']:
        iq_files = glob.glob(f"{data_dir}/iq_samples/*.dat")
        info_files = glob.glob(f"{data_dir}/iq_samples/*.txt")
        iq_channel = bot.get_channel(IQ_CHANNEL_ID)
        count = await upload_files(iq_channel, iq_files + info_files, "IQ")
        total_uploaded += count

    if file_type in ['snr', 'all']:
        snr_files = glob.glob(f"{data_dir}/snr_logs/*.csv")
        snr_channel = bot.get_channel(SNR_CHANNEL_ID)
        count = await upload_files(snr_channel, snr_files, "SNR")
        total_uploaded += count

    print(f"Upload complete! {total_uploaded} new files uploaded.")
    await bot.close()  # Close the bot after uploads are done

# Run the bot
bot.run(TOKEN)