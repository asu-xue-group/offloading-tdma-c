library(tidyverse)

# Read the data
delay <- read_csv("results-delay.csv") |> mutate(source = "delay")
relay <- read_csv("results-relay.csv") |> mutate(source = "relay")

# Combine datasets
data <- bind_rows(delay, relay) |>
  mutate(size = str_extract(tc, "small|medium|bigger|big"))

# Desired size order
size_levels <- c("small", "medium", "big", "bigger")

# Compute means per group & source
summary_df <- data |>
  group_by(source, size, mode) |>
  summarize(
    mean_reward = mean(reward, na.rm = TRUE),
    mean_time = mean(time, na.rm = TRUE),
    .groups = "drop"
  )

# Pivot wider to have delay and relay side by side
comparison_df <- summary_df |>
  pivot_wider(
    names_from = source,
    values_from = c(mean_reward, mean_time)
  ) |>
  mutate(
    reward_pct = mean_reward_delay / mean_reward_relay,
    time_pct   = mean_time_delay   / mean_time_relay,
    size = factor(size, levels = size_levels)
  ) |>
  select(size, mode,
         # relay_reward = mean_reward_relay,
         # delay_reward = mean_reward_delay,
         reward_pct,
         # relay_time   = mean_time_relay,
         # delay_time   = mean_time_delay,
         time_pct) |>
  arrange(size, mode)

# Print the table
print(comparison_df)

# Optional: nicer table
library(knitr)
kable(comparison_df, digits = 3, caption = "Relay vs Delay: Percentage Change (Relay as Reference) by Size and Mode")
