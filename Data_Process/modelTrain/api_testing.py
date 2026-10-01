import openai

openai.api_key = "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"

try:
    response = openai.ChatCompletion.create(
        model="gpt-4o",
        messages=[{"role": "system", "content": "You're an assistant"},
                  {"role": "user", "content": "Test if OpenAI API is avaliable"}]
    )
    print(response["choices"][0]["message"]["content"])
except Exception as e:
    print(f"API request failed: {e}")
