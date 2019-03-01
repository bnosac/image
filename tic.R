#if (Sys.getenv("id_rsa") != "" && ci()$get_branch() == "master") {
# pkgdown documentation can be built optionally. Other example criteria:
# - `inherits(ci(), "TravisCI")`: Only for Travis CI
# - `ci()$is_tag()`: Only for tags, not for branches
# - `Sys.getenv("BUILD_PKGDOWN") != ""`: If the env var "BUILD_PKGDOWN" is set
# - `Sys.getenv("TRAVIS_EVENT_TYPE") == "cron"`: Only for Travis cron jobs

get_stage("before_install") %>%
  add_code_step(install.packages(c("rcmdcheck", "curl", "drat", "git2r", "knitr", "withr", "pkgbuild", "openssl"))) %>%
  add_code_step(install.packages(c("Rcpp", "magick", "pixmap", "dlib", "sp")))

get_stage("install") %>% add_code_step(cat("Nothing to do"))
get_stage("after_install") %>% add_code_step(cat("Nothing to do"))
get_stage("before_script") %>% add_code_step(cat("Nothing to do"))

get_stage("script") %>%
  add_code_step(rcmdcheck::rcmdcheck("image.Otsu", args = "--no-manual")) %>%
  add_code_step(rcmdcheck::rcmdcheck("image.CornerDetectionF9", args = "--no-manual")) %>%
  #add_code_step(rcmdcheck::rcmdcheck("image.CornerDetectionHarris", args = "--no-manual")) %>%
  #add_code_step(rcmdcheck::rcmdcheck("image.dlib", args = "--no-manual")) %>%
  add_code_step(rcmdcheck::rcmdcheck("image.DenoiseNLMeans", args = "--no-manual")) %>%
  #add_code_step(rcmdcheck::rcmdcheck("image.darknet", args = "--no-manual")) %>%
  #add_code_step(rcmdcheck::rcmdcheck("image.CannyEdges", args = "--no-manual")) %>%
  #add_code_step(rcmdcheck::rcmdcheck("image.OpenPano", args = "--no-manual")) %>%
  add_code_step(rcmdcheck::rcmdcheck("image.ContourDetector", args = "--no-manual")) %>%
  add_code_step(rcmdcheck::rcmdcheck("image.LineSegmentDetector", args = "--no-manual"))
  

get_stage("before_deploy") %>%
  add_step(step_setup_ssh())

get_stage("deploy") %>%
  add_step(step_setup_push_deploy(path = "~/git/drat", branch = "gh-pages", remote = "git@github.com:bnosac/drat.git")) %>%
  add_code_step(drat::insertPackage(pkgbuild::build("image.Otsu", binary = (getOption("pkgType") != "source")))) %>%
  add_code_step(drat::insertPackage(pkgbuild::build("image.CornerDetectionF9", binary = (getOption("pkgType") != "source")))) %>%
  #add_code_step(drat::insertPackage(pkgbuild::build("image.CornerDetectionHarris", binary = (getOption("pkgType") != "source")))) %>%
  add_code_step(drat::insertPackage(pkgbuild::build("image.dlib", binary = (getOption("pkgType") != "source")))) %>%
  add_code_step(drat::insertPackage(pkgbuild::build("image.DenoiseNLMeans", binary = (getOption("pkgType") != "source")))) %>%
  #add_code_step(drat::insertPackage(pkgbuild::build("image.darknet", binary = (getOption("pkgType") != "source")))) %>%
  #add_code_step(drat::insertPackage(pkgbuild::build("image.CannyEdges", binary = (getOption("pkgType") != "source")))) %>%
  #add_code_step(drat::insertPackage(pkgbuild::build("image.OpenPano", binary = (getOption("pkgType") != "source")))) %>%
  add_code_step(drat::insertPackage(pkgbuild::build("image.ContourDetector", binary = (getOption("pkgType") != "source")))) %>%
  add_code_step(drat::insertPackage(pkgbuild::build("image.LineSegmentDetector", binary = (getOption("pkgType") != "source")))) %>%
  add_step(step_do_push_deploy(path = "~/git/drat"))

get_stage("after_deploy") %>%
  add_code_step(knitr::knit("~/git/drat/index.Rmd", "~/git/drat/index.md")) %>%
  add_code_step(knitr::knit("~/git/drat/index.Rmd", "~/git/drat/README.md")) %>%
  add_step(step_do_push_deploy(path = "~/git/drat"))
