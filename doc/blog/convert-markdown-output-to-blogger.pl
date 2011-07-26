#!/usr/bin/perl -w

# This is a small Perl script that I use to convert the markdown output
# into a format that I can paste into the blogger editor.

my %imgurls = (
    'music-box-overview.png'
    => 'http://4.bp.blogspot.com/-mpoqLq3MqMs/TjVFkyhllSI/AAAAAAAAADc/-zBEqOknCl8/s1600/music-box-overview.png',
    'Audio2DJ.jpg'
    => 'http://1.bp.blogspot.com/-lFS55wFpTwY/TjQsn4cHGKI/AAAAAAAAADU/h2xqmpMA40I/s1600/Audio2DJ.jpg'
);


sub makeImgTag {
    my ( $filename ) = @_;
    my $url = $imgurls{$filename};
    return "<div class=\"separator\" style=\"clear: both; text-align: center;\">\n" .
        "<a href=\"$url\" imageanchor=\"1\" style=\"margin-left:1em; margin-right:1em\">\n" .
        "<img border=\"0\" width=\"320\" src=\"$url\" />\n".
        "</a>\n</div>\n";
}

while (<>) {
    # Step 1: Replace any <img> tag with blogspot's <div><img>
    s/<img src="img\/([^"]*)"[^>]*>/makeImgTag($1)/e;
    # Step 2: Strip <h1>, as the blog title will be displayed as <h1> in blogspot
    s/<h1>.*?<\/h1>//;
    # Step 3: Strip newlines, as newlines are confusing in blogspot
    # chomp; # commented out, because it would also strip newlines in example code
    # done.
    print;
}
