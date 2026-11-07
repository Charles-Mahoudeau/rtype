-- The version of the engine and of the commit it is built from. The runtime gets them as defines (`--version`), the
-- release package reads them for its name and its manifest, so they cannot disagree.
import("core.project.config")
import("core.project.project")

-- The version in `set_version`, e.g. "0.1.0".
function base()
    return project.version() or "0.0.0"
end

-- The version suffix of this build: "nightly.<commit>" when configured with `--nightly=<commit>`, or what the environment
-- variable AYLO_VERSION_SUFFIX holds (the release workflow sets it: "nightly.<commit>", "dev.<commit>", empty for a tag).
function suffix()
    local given = os.getenv("AYLO_VERSION_SUFFIX")
    if given and #given > 0 then
        return given
    end
    local nightly = config.get("nightly")
    if nightly and #nightly > 0 then
        return "nightly." .. nightly
    end
    return ""
end

-- The version of this build: "0.1.0", or "0.1.0-nightly.<commit>" for a nightly.
function full()
    local extra = suffix()
    return #extra > 0 and (base() .. "-" .. extra) or base()
end

-- The commit the build is made from: RTYPE_COMMIT if set (a build without git), else `git rev-parse`, else "".
function commit()
    local given = os.getenv("RTYPE_COMMIT")
    if given and #given > 0 then
        return given
    end
    local sha = try { function() return os.iorunv("git", {"rev-parse", "--short=8", "HEAD"}, {curdir = os.projectdir()}) end }
    return sha and sha:trim() or ""
end
