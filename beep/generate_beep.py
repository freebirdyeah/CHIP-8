import math
import wave
import struct
from pathlib import Path

SAMPLE_RATE = 44100  # samples per second
DURATION = 0.25      # seconds
FREQUENCY = 440.0    # Hz
AMPLITUDE = 0.5      # 0.0 to 1.0

asset_path = Path("assets")
asset_path.mkdir(parents=True, exist_ok=True)

output_file = asset_path / "beep.wav"
num_samples = int(SAMPLE_RATE * DURATION)
with wave.open(str(output_file), "w") as wav_file:
    wav_file.setnchannels(1)  # mono
    wav_file.setsampwidth(2)  # 16-bit PCM
    wav_file.setframerate(SAMPLE_RATE)

    for n in range(num_samples):
        t = n / SAMPLE_RATE
        value = AMPLITUDE * math.sin(2.0 * math.pi * FREQUENCY * t)
        packed_value = struct.pack('<h', int(value * 32767))
        wav_file.writeframes(packed_value)

print(f"Generated {output_file}")
