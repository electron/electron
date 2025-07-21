const binding = process._linkedBinding('electron_utility_local_ai_handler');

export const setPromptAPIHandler = binding.setPromptAPIHandler;
