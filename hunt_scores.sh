#!/bin/bash

declare -A user_sums

# Run the command and process line by line
while IFS= read -r line; do
    # Look for lines containing treasure data
    if [[ "$line" == Treasure\ ID:* ]]; then
        # Extract user
        user=$(echo "$line" | grep -oP 'User: \K[^ ]+')

        # Extract value
        value=$(echo "$line" | grep -oP 'Value: \K[0-9]+')

        # Update the user's sum
        if [[ -n "$user" && -n "$value" ]]; then
            user_sums["$user"]=$(( user_sums["$user"] + value ))
        fi
    fi
done < <(./treasure_manager --list $1)


echo "Total scores for hunt $1:"
# Print results
for user in "${!user_sums[@]}"; do
    echo "$user: ${user_sums[$user]}"
done
