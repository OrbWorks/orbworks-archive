using System;
using System.Collections;
using System.Xml;
using System.IO;
using System.Text;

namespace DocTool {
	class Program {
		static void Usage() {
			Console.WriteLine("DocTool.exe - Documentation processor");
			Console.WriteLine("  -product docs.xml out.xml prod \"Product Name\"");
			Console.WriteLine("  -chmsplit docs.html outdir");
			Console.WriteLine("  -chmindex project.hhp");
			Console.WriteLine("  -booksplit docs.html outdir");
			Console.WriteLine("  -quickdocs docs.xml out.txt");
		}

		static string ReadFile(string fileName) {
			using (StreamReader sr = new StreamReader(fileName))
				return sr.ReadToEnd();
		}

		static string GetAttrib(string contents, int start, string name) {
			name += "=\"";
			int s_attrib = contents.IndexOf(name, start);
			if (s_attrib == -1)
				return "";
			s_attrib += name.Length;
			int e_attrib = contents.IndexOf('"', s_attrib);
			return contents.Substring(s_attrib, e_attrib - s_attrib);
		}

		static string CleanString(string str) {
			str = str.Replace('\n', ' ');
			str = str.Replace('\t', ' ');
			str = str.Replace('\r', ' ');
			string newStr = str.Replace("  ", " ");
			while (newStr != str) {
				str = newStr;
				newStr = str.Replace("  ", " ");
			}
			return newStr.Trim();
		}

		static void QuickDocs(string infile, string outfile) {
			ArrayList keys = new ArrayList();
			ArrayList docs = new ArrayList();

			XmlDocument doc = new XmlDocument();
			XmlTextReader reader = new XmlTextReader(infile);
			doc.Load(reader);

			// functions
			XmlNodeList nodes = doc.SelectNodes("/Documentation/book/category/method");
			foreach (XmlNode node in nodes) {
				keys.Add(node.Attributes.GetNamedItem("name").Value);
				docs.Add(CleanString(node.SelectSingleNode("desc").InnerText));
			}

			// objects
			nodes = doc.SelectNodes("/Documentation/book/category/object");
			foreach (XmlNode objNode in nodes) {
				// object docs
				string objName = objNode.Attributes.GetNamedItem("name").Value;
				// TODO: use sdesc when available or always use desc?
				XmlNode descNode = objNode.SelectSingleNode("sdesc");
				if (descNode == null)
					descNode = objNode.SelectSingleNode("desc");
				keys.Add(objName);
				docs.Add(CleanString(descNode.InnerText));

				// properties
				XmlNodeList propNodes = objNode.SelectNodes("property");
				foreach (XmlNode propNode in propNodes) {
					keys.Add(objName + "." + propNode.Attributes.GetNamedItem("name").Value);
					docs.Add(CleanString(propNode.SelectSingleNode("desc").InnerText));
				}

				// methods
				XmlNodeList methodNodes = objNode.SelectNodes("method");
				foreach (XmlNode methodNode in methodNodes) {
					keys.Add(objName + "." + methodNode.Attributes.GetNamedItem("name").Value);
					docs.Add(CleanString(methodNode.SelectSingleNode("desc").InnerText));
				}
			}

			StreamWriter sw = new StreamWriter(outfile, false);
			for (int i = 0; i < keys.Count; i++) {
				sw.WriteLine((string)keys[i]);
				sw.WriteLine((string)docs[i]);
			}

			sw.Close();
		}

		static void Product(string infile, string outfile, string prodkey, string prodname) {
			StreamReader sr = new StreamReader(infile);
			StreamWriter sw = new StreamWriter(outfile, false);

			string line;
			bool skip = false;
			string skipNode = "";
			while ((line = sr.ReadLine()) != null) {
				if (line.IndexOf("<book") != -1) {
					string bookprod = GetAttrib(line, line.IndexOf("<book"), "product");
					if (bookprod != "" && bookprod != prodkey) {
						skip = true;
						skipNode = "</book>";
					}
				} else if (line.IndexOf("<topic") != -1) {
					string bookprod = GetAttrib(line, line.IndexOf("<topic"), "product");
					if (bookprod != "" && bookprod != prodkey) {
						skip = true;
						skipNode = "</topic>";
					}
				} else if (skip && line.IndexOf(skipNode) != -1) {
					skip = false;
					continue;
				}

				if (!skip) {
					line = line.Replace("{product}", prodname);
					sw.WriteLine(line);
				}
			}
			sw.Close();
			sr.Close();
		}

		static string ProcessImports(string contents) {
			int iimport = contents.IndexOf("<import name=");
			if (iimport != -1) {
				StringBuilder sb = new StringBuilder();
				sb.Append(contents.Substring(0, iimport));
				int start = iimport + 14;
				int end = contents.IndexOf('"', start);
				string importName = contents.Substring(start, end - start);
				string importContents = ReadFile(importName);
				sb.Append(importContents);
				start = contents.IndexOf("</import>", start);
				start += 9;
				sb.Append(contents.Substring(start));
				return sb.ToString();

			} else {
				return contents;
			}
		}

		static void ProcessChmSplitFile(string path, string contents) {
			StreamWriter sw = new StreamWriter(path, false);
			contents = ProcessImports(contents);
			sw.Write(contents);
			sw.Close();
		}

		static void ChmSplit(string infile, string outdir) {
			string docsHtml = ReadFile(infile);
			int start = 0;
			while ((start = docsHtml.IndexOf("<file name=", start)) != -1) {
				start += 12;
				int end = docsHtml.IndexOf('"', start);
				string fileName = docsHtml.Substring(start, end - start);
				string path = outdir + "\\" + fileName;
				start = docsHtml.IndexOf('>', end) + 1;
				end = docsHtml.IndexOf("</file>", start);
				ProcessChmSplitFile(path, docsHtml.Substring(start, end - start));
			}
		}

		static void ProcessChmIndexFile(string fileName, StreamWriter sw) {
			string contents = ReadFile(fileName);

			int start = contents.IndexOf("<title");
			if (start != -1) {
				int s_title = contents.IndexOf('>', start) + 1;
				int e_title = contents.IndexOf('<', s_title);
				string title = contents.Substring(s_title, e_title - s_title);
				sw.Write("<LI><OBJECT type=\"text/sitemap\"><param name=\"Name\" value=\"" + title + "\">\n" +
					"<param name=\"Name\" value=\"" + title + "\"><param name=\"Local\" value=\"" +
					fileName + "\"></OBJECT>\n");

				string keyword = GetAttrib(contents, start, "keywords");
				if (keyword == "")
					return;

				string[] keywords = keyword.Split(',');
				foreach (string key in keywords){
					if (key.Length == 0)
						continue;
					sw.Write("<LI><OBJECT type=\"text/sitemap\"><param name=\"Name\" value=\"" + key + "\">\n" +
						"<param name=\"Name\" value=\"" + key + "\"><param name=\"Local\" value=\"" +
						fileName + "\"></OBJECT>\n");
				}
			}
		}

		static void ChmIndex(string projFile) {
			string indexFile = projFile.Substring(0, projFile.Length - 1);
			indexFile += 'k';

			StreamReader sr = new StreamReader(projFile);
			StreamWriter sw = new StreamWriter(indexFile);

			sw.Write(
				"<HTML>\n<HEAD>\n<meta name=\"GENERATOR\" content=\"Microsoft&reg; HTML Help Workshop 4.1\">\n" +
				"<!-- Sitemap 1.0 -->\n</HEAD>\n<BODY>\n<OBJECT type=\"text/site properties\">\n</OBJECT>\n<UL>\n"
			);
			string line;
			while ((line = sr.ReadLine()) != "[FILES]") {
				if (line == null)
					break;
			}
			while ((line = sr.ReadLine()) != null) {
				if (line.IndexOf(".html") != -1) {
					ProcessChmIndexFile(line, sw);
				}
			}
			sw.WriteLine("</UL></BODY></HTML>");
			sw.Close();
		}

		static Hashtable bookTable; // bookName -> StringBuilder
		static Hashtable fileTable; // fileName -> bookName
		static ArrayList imageList; // all images referenced

		static void AddImages(string contents) {
			int start, end = 0;
			while ((start = contents.IndexOf("<img ", end)) != -1) {
				string fileName = GetAttrib(contents, start, "src");
				start = contents.IndexOf('>', start) + 1;
				end = contents.IndexOf(">", start);
				imageList.Add(fileName);
			}
		}

		static void AddToBook(string bookName, string fileName, string contents) {
			fileTable.Add(fileName, bookName);
			contents = ProcessImports(contents);
			if (bookTable.ContainsKey(bookName)) {
				// additional file - add hr and anchor and copy just inside <body>
				StringBuilder sb = (StringBuilder)bookTable[bookName];
				sb.Append("<a name=\"" + fileName.Substring(0, fileName.Length - 5) + "\"><hr/></a>\n");
				int start = contents.IndexOf("<body");
				start = contents.IndexOf('>', start) + 1;
				int end = contents.IndexOf("</body>", start);
				sb.Append(contents.Substring(start, end - start));
			} else {
				// first file - copy up to the </body>
				StringBuilder sb = new StringBuilder();
				int end = contents.IndexOf("</body>");
				sb.Append(contents.Substring(0, end));
				bookTable.Add(bookName, sb);
			}
		}

		static void BookSplit(string infile, string outdir) {
			DateTime timeStart = DateTime.Now;
			bookTable = new Hashtable();
			fileTable = new Hashtable();
			imageList = new ArrayList();
			string contents = ReadFile(infile);
			int start, end = 0;
			while ((start = contents.IndexOf("<file name=", end)) != -1) {
				string fileName = GetAttrib(contents, start, "name");
				string bookName = GetAttrib(contents, start, "book");

				start = contents.IndexOf('>', start) + 1;
				end = contents.IndexOf("</file>", start);
				AddToBook(bookName, fileName, contents.Substring(start, end - start));
			}
			// add </body> to each book
			foreach (StringBuilder sb in bookTable.Values)
				sb.Append("</body>");

			// find referenced images
			foreach (StringBuilder sb in bookTable.Values)
				AddImages(sb.ToString());

            // replace links with book relative links
			foreach (string bookName in bookTable.Keys) {
				StringBuilder sb = (StringBuilder)bookTable[bookName];
				string bookFileName;
				if (bookName.IndexOf('-') != -1)
					bookFileName = "c" + bookName.Substring(bookName.IndexOf('-') + 1) + ".html";
				else
					bookFileName = "b" + bookName + ".html";

				foreach (string fileName in fileTable.Keys) {
					string link = "href=\"" + fileName;
					string relLink;
					if ((string)fileTable[fileName] == bookName) {
						if (fileName != bookFileName)
							relLink = "href=\"#" + fileName.Substring(0, fileName.Length - 5);
						else
							relLink = "href=\"#";
					} else {
						relLink = "href=\"" + (string)fileTable[fileName] + ".html";
						if (fileName[0] != 'b' && fileName[0] != 'c')
							relLink += "#" + fileName.Substring(0, fileName.Length - 5);
					}
					sb.Replace(link, relLink);
				}
			}

			// write books
			foreach (string bookName in bookTable.Keys) {
				using (StreamWriter sw = new StreamWriter(outdir + "\\" + bookName + ".html", false)) {
					sw.Write(bookTable[bookName]);
				}
			}

			// copy referenced images
			foreach (string image in imageList) {
				File.Copy(image, outdir + "\\" + image, true);
			}

			TimeSpan timeSpan = DateTime.Now - timeStart;
			Console.WriteLine("booksplit took {0} seconds", timeSpan.TotalMilliseconds / 1000);
		}

		static void Main(string[] args) {
			if (args.Length < 1) {
				Usage();
				return;
			}

			if (args[0] == "-quickdocs") {
				if (args.Length == 3) {
					QuickDocs(args[1], args[2]);
				} else {
					Usage();
				}
			} else if (args[0] == "-product") {
				if (args.Length == 5) {
					Product(args[1], args[2], args[3], args[4]);
				} else {
					Usage();
				}
			} else if (args[0] == "-chmsplit") {
				if (args.Length == 3) {
					ChmSplit(args[1], args[2]);
				} else {
					Usage();
				}
			} else if (args[0] == "-chmindex") {
				if (args.Length == 2) {
					ChmIndex(args[1]);
				} else {
					Usage();
				}
			} else if (args[0] == "-booksplit") {
				if (args.Length == 3) {
					BookSplit(args[1], args[2]);
				} else {
					Usage();
				}
			} else {
				Usage();
			}
		}
	}
}
