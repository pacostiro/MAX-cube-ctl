class MaxCtlJNI
{
    static
    {
        System.loadLibrary("maxctl");
    }

    private native void maxctl2();

    public static void main(String[] args)
    {
        new MaxCtlJNI().maxctl2();
    }

}
