# This is base on the `get_state_dir` in python/mach/mach/utils.py
function __mach_get_state_dir -a source_dir
    set -l dir_basename (path basename "$source_dir")
    set -l dir_hash (string sub -l 12 (sha256 -s "$source_dir"))
    echo "$HOME/.mozbuild/srcdirs/$dir_basename-$dir_hash"
end

function __mach_find_completion_script -a executable_path
    # Prefer the directory hinted by the executable path (./mach, ../mach, etc.)
    set -l search_dir (path dirname $executable_path)
    set -l state_dir (__mach_get_state_dir $search_dir)
    if test -d "$state_dir"
        echo "$state_dir/completions/mach.fish"
        return
    end

    # Otherwise, walk upward from the current working directory
    set search_dir (pwd)
    while test "$search_dir" != /
        set state_dir (__mach_get_state_dir "$search_dir")
        if test -d "$state_dir"
            echo "$state_dir/completions/mach.fish"
            return
        end
        set search_dir (path dirname "$search_dir")
    end

    return 1
end

function __mach_dynamic_completion
    set -l command_name (commandline -opc)[1]
    set -l executable_path (realpath "$(command -v "$command_name")")

    set -l com_script (__mach_find_completion_script $executable_path); or return

    # Execute the completion script found
    fish --no-config --private -c "source $com_script; complete -C \"$(commandline)\""
end

complete -c mach -f -a "(__mach_dynamic_completion)"
