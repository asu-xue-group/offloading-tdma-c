library(tidyverse)

# Load datasets
tdma <- read_csv("tdma.csv")
ofdma <- read_csv("ofdma.csv")

# Join by matching case identifiers
df <- inner_join(tdma, ofdma, by = c("tc" = "file")) |>
  rename(reward_tdma = reward.x, reward_ofdma = reward.y)

# Filter out cases where OFDMA reward is zero to avoid division by zero
df <- df |> filter(reward_ofdma > 0)

# Determine who performed better
df <- df |> mutate(
  outcome = case_when(
    reward_tdma > reward_ofdma ~ "TDMA better",
    reward_tdma < reward_ofdma ~ "OFDMA better",
    TRUE ~ "Tie"
  )
)

# Add category based on 'tc' field (assumes names like 'small_XX' or 'medium_XX')
df <- df |> mutate(category = case_when(
  str_starts(tc, "small") ~ "small",
  str_starts(tc, "medium") ~ "medium",
  TRUE ~ "unknown"
))

# Split and count by category
df_counts <- df |> count(category, outcome) |> pivot_wider(names_from = outcome, values_from = n, values_fill = 0)
print(df_counts)

# Compute average percentage improvement by category
df_summary <- df |> 
  mutate(improvement_pct = (reward_tdma - reward_ofdma) / reward_ofdma * 100) |>
  group_by(category) |>
  summarize(avg_improvement_pct = mean(improvement_pct, na.rm = TRUE), .groups = "drop")
print(df_summary)

# Print rows where OFDMA is better
cat("Cases where OFDMA is better:\n")
print(df |> filter(outcome == "OFDMA better") |> select(tc, reward_tdma, reward_ofdma))

# Print rows where OFDMA tied with TDMA
cat("\nCases where TDMA and OFDMA tie:\n")
print(df |> filter(outcome == "Tie") |> select(tc, reward_tdma, reward_ofdma))
