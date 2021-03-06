package com.pivotal.hawq.mapreduce;

import com.google.common.collect.Lists;
import com.google.common.collect.Maps;
import com.google.common.io.CharStreams;

import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.List;
import java.util.Map;

public class MRFormatConfiguration {

	///////////////////////////////////////////////////////
	//// test database info
	///////////////////////////////////////////////////////
	public static final String	TEST_DB_NAME	= "gptest";
	public static final String	TEST_DB_HOST	= getMasterHost();
	public static final int		TEST_DB_PORT	= getMasterPort();
	public static final String	TEST_DB_URL		= TEST_DB_HOST + ":" + TEST_DB_PORT + "/" + TEST_DB_NAME;

	// specify a folder where all unit test outputs go to
	public static final File TEST_FOLDER = new File("test-data");

	// specify a folder where all feature test outputs go to
	public static final File FT_TEST_FOLDER = new File("feature-test-data");

	// type -> values
	public static Map<String, List<String>> DATA_SET;

	static {
		// load data set
		DATA_SET = Maps.newHashMap();
		try {
			InputStream is = MRFormatConfiguration.class.getResourceAsStream("/dataset");
			List<String> lines = CharStreams.readLines(new InputStreamReader(is));
			for (String line : lines) {
				List<String> values = Lists.newArrayList();
				String[] strs = line.split("  ");
				for (int i = 1; i < strs.length; i++) {
					if (!strs[i].equals("#")) {
						values.add(strs[i]);
					}
				}
				DATA_SET.put(strs[0], values);
			}
			is.close();

		} catch (Exception e) {
			throw new ExceptionInInitializerError(e);
		}

		// create output folder when not exist
		if (!TEST_FOLDER.exists()) {
			TEST_FOLDER.mkdir();
		}
		if (!FT_TEST_FOLDER.exists()) {
			FT_TEST_FOLDER.mkdir();
		}
	}

	private static final String DEFAULT_MASTER_HOST = "localhost";
	private static final int	DEFAULT_MASTER_PORT	= 5432;

	private static String getMasterHost() {
		Map<String, String> env = System.getenv();
		if (env.containsKey("PG_BASE_ADDRESS"))
			return env.get("PG_BASE_ADDRESS");
		else
			return DEFAULT_MASTER_HOST;
	}

	private static int getMasterPort() {
		Map<String, String> env = System.getenv();
		if (env.containsKey("PG_BASE_PORT"))
			return Integer.parseInt(env.get("PG_BASE_PORT"));
		else
			return DEFAULT_MASTER_PORT;
	}

}
