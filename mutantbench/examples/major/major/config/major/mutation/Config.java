// This package name is required by Major!
package major.mutation;

import java.util.List;
import java.util.ArrayList;
import java.util.Set;
import java.util.TreeSet;

/**
 * A simple driver class for Major --
 * this class name is required by Major!
 */
public class Config {

    /*
     * The mutant identifier:
     *
     * __M_NO <  0 -> Run original version
     *
     * __M_NO == 0 -> Run original version and gather coverage information
     *
     * __M_NO >  0 -> Execute mutant with the corresponding id
     */
    public static int __M_NO = -1;

    // Set to store IDs of covered mutants
    public static Set<Integer> covSet = new TreeSet<Integer>();

    // The coverage method is called if and only if the
    // mutant identifier is set to 0!
    public static boolean COVERED(int from, int to) {
        synchronized (covSet) {
            for (int i=from; i<=to; ++i) {
                covSet.add(i);
            }
        }
        // Always return false as required by
        // Conditional Mutation!
        return false;
    }

    /*
     * Additional methods for the mutation analysis back-end
     */
    // Reset the coverage information
    public static void reset() {
        synchronized (covSet) {
            covSet.clear();
        }
    }

    // Get list of all covered mutants
    public static List<Integer> getCoverageList() {
        synchronized (covSet) {
            return new ArrayList<Integer>(covSet);
        }
    }
}
